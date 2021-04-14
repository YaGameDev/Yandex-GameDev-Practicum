#include "StdAfx.h"
#include "Player.h"

#include <CrySchematyc/Reflection/TypeDesc.h>
#include <CrySchematyc/Utils/EnumFlags.h>
#include <CrySchematyc/Env/IEnvRegistry.h>
#include <CrySchematyc/Env/IEnvRegistrar.h>
#include <CrySchematyc/Env/Elements/EnvComponent.h>
#include <CrySchematyc/Env/Elements/EnvFunction.h>
#include <CrySchematyc/Env/Elements/EnvSignal.h>
#include <CrySchematyc/ResourceTypes.h>
#include <CrySchematyc/MathTypes.h>
#include <CrySchematyc/Utils/SharedString.h>
#include <CryCore/StaticInstanceList.h>

#include <DefaultComponents/Physics/RigidBodyComponent.h>
#include <DefaultComponents/Input/InputComponent.h>

void Player::Initialize()
{
	m_character = m_pEntity->GetOrCreateComponent<Cry::DefaultComponents::CCharacterControllerComponent>();
	m_camera = m_pEntity->GetOrCreateComponent<Cry::DefaultComponents::CCameraComponent>();
	m_input = m_pEntity->GetOrCreateComponent<Cry::DefaultComponents::CInputComponent>();


	m_character->SetTransformMatrix(Matrix34::Create(Vec3(1.f), IDENTITY, Vec3(0, 0, 1.f)));

	m_input->RegisterAction("player", "moveleft", [this](int activationMode, float value) { HandleInputFlagChange(EInputFlag::MoveLeft, (EActionActivationMode)activationMode);  });
	m_input->BindAction("player", "moveleft", eAID_KeyboardMouse, EKeyId::eKI_A);

	m_input->RegisterAction("player", "moveright", [this](int activationMode, float value) { HandleInputFlagChange(EInputFlag::MoveRight, (EActionActivationMode)activationMode);  });
	m_input->BindAction("player", "moveright", eAID_KeyboardMouse, EKeyId::eKI_D);

	m_input->RegisterAction("player", "moveforward", [this](int activationMode, float value) { HandleInputFlagChange(EInputFlag::MoveForward, (EActionActivationMode)activationMode);  });
	m_input->BindAction("player", "moveforward", eAID_KeyboardMouse, EKeyId::eKI_W);

	m_input->RegisterAction("player", "moveback", [this](int activationMode, float value) { HandleInputFlagChange(EInputFlag::MoveBack, (EActionActivationMode)activationMode);  });
	m_input->BindAction("player", "moveback", eAID_KeyboardMouse, EKeyId::eKI_S);

	m_input->RegisterAction("player", "mouse_rotateyaw", [this](int activationMode, float value) { m_mouseDelta.x -= value; });
	m_input->BindAction("player", "mouse_rotateyaw", eAID_KeyboardMouse, EKeyId::eKI_MouseX);

	m_input->RegisterAction("player", "mouse_rotatepitch", [this](int activationMode, float value) { m_mouseDelta.y -= value; });
	m_input->BindAction("player", "mouse_rotatepitch", eAID_KeyboardMouse, EKeyId::eKI_MouseY);

	m_input->RegisterAction("player", "shoot", [this](int activationMode, float value) {
		if (activationMode == eAAM_OnPress)
			pickObject();
	});

	m_input->BindAction("player", "shoot", eAID_KeyboardMouse, EKeyId::eKI_Mouse1);
}

Cry::Entity::EventFlags Player::GetEventMask() const
{
	return Cry::Entity::EEvent::Update;// | Cry::Entity::EEvent::PrePhysicsUpdate;
}

void Player::ProcessEvent(const SEntityEvent& event)
{
	switch (event.event)
	{
	case Cry::Entity::EEvent::Update:
	{
		const float delta = event.fParam[0];

		updateMovement(delta);
		updateCamera(delta);
		updateGrabbedObject(delta);
	}
	break;
	}
}


void Player::updateMovement(float delta)
{
	if (!m_character->IsOnGround())
		return;

	Vec3 velocity = ZERO;
	const float moveSpeed = 40.0f;

	if (m_inputFlags & EInputFlag::MoveLeft)
		velocity.x -= 1.f;
	
	if (m_inputFlags & EInputFlag::MoveRight)
		velocity.x += 1.f;

	if (m_inputFlags & EInputFlag::MoveForward)
		velocity.y += 1.f;
	
	if (m_inputFlags & EInputFlag::MoveBack)
		velocity.y -= 1.f;
	
	if (velocity == ZERO)
		return;

	velocity.normalize();

	m_character->AddVelocity(m_bodyOrientation * velocity * moveSpeed * delta);
}

void Player::updateCamera(float delta)
{
	const float cameraHeight = 0.7;
	const float rotationSpeed = 0.001f;
	const float rotationLimitsMinPitch = -1.55f;
	const float rotationLimitsMaxPitch = 1.55f;

	Matrix33 camOrientation;
	m_camera->GetTransformMatrix().GetRotation33(camOrientation);
	Ang3 ypr = CCamera::CreateAnglesYPR(camOrientation);

	ypr.x += m_mouseDelta.x * rotationSpeed;
	ypr.y = CLAMP(ypr.y + m_mouseDelta.y * rotationSpeed, rotationLimitsMinPitch, rotationLimitsMaxPitch);
	ypr.z = 0;

	m_mouseDelta = ZERO;

	m_bodyOrientation = CCamera::CreateOrientationYPR(Ang3(ypr.x, 0, 0));

	Matrix34 localTransform = m_character->GetTransformMatrix();
	camOrientation = CCamera::CreateOrientationYPR(ypr);
	localTransform.SetRotation33(camOrientation);
	localTransform.AddTranslation(Vec3(0, 0, cameraHeight));

	m_camera->SetTransformMatrix(localTransform);

	m_cameraViewDir = camOrientation * FORWARD_DIRECTION;
}

void Player::updateGrabbedObject(float delta)
{
	if (!m_grabbedObject)
		return;

	Vec3 origin = m_camera->GetWorldTransformMatrix().GetTranslation();
	m_grabbedObject->SetPos(origin + m_cameraViewDir * GRAB_OBJECT_DIST);

	AABB box;
	m_grabbedObject->GetWorldBounds(box);

	IPersistantDebug* db = gEnv->pGameFramework->GetIPersistantDebug();
	db->Begin("AABB", true);
	db->AddAABB(box.min, box.max, ColorF(1, 0, 0), 1.0);
}

void Player::doPerspectiveScaling(IEntity* entity) {
	const float maxDist = 20;

	AABB box;
	m_grabbedObject->GetWorldBounds(box);

	std::vector<Vec3> points = { 
		{box.min.x, box.min.y, box.min.z},
		{box.max.x, box.min.y, box.min.z},
		{box.max.x, box.max.y, box.min.z},
		{box.min.x, box.max.y, box.min.z},

		{box.min.x, box.min.y, box.max.z},
		{box.max.x, box.min.y, box.max.z},
		{box.max.x, box.max.y, box.max.z},
		{box.min.x, box.max.y, box.max.z},
	};

	Vec3 origin = m_camera->GetWorldTransformMatrix().GetTranslation();
	box.GetSize();
	ray_hit hit;

	float minDist = 2 * maxDist;
	int minRay = -1;

	for (int j = 0; j < 1000; j++) {
		for (size_t i = 0; i < points.size(); i++) {
			Vec3 dir = (points[i] - origin).normalize() * maxDist;
			rayCastFromCamera(hit, dir, ent_all);

			if (hit.pCollider && hit.dist < minDist) {
				minDist = hit.dist;
				minRay = i;
			}
		}
	}
	
	if (minRay == -1)
		return;

	Vec3 old = points[minRay] - origin;
	Vec3 dir = old.normalized();

	float oldP = old.dot(m_cameraViewDir);
	float newP = oldP * minDist / old.len();

	float d1 = GRAB_OBJECT_DIST;
	float f = newP;
	float d2 = f / (1 + (oldP - d1) / d1);

	float k = d2 / d1;

	Vec3 newPos = origin + m_cameraViewDir * d1 * k;

	{
		IPersistantDebug* db = gEnv->pGameFramework->GetIPersistantDebug();
		db->Begin("scale", false);
		db->AddSphere(origin + m_cameraViewDir * oldP, 0.05f, ColorF(0, 0, 1), 4.0);
		db->AddSphere(origin + m_cameraViewDir * newP, 0.05f, ColorF(1, 0, 1), 4.0);
	}

	entity->SetPos(newPos);
	entity->SetScale(Vec3(k));

	//const float radius = 0.5f;

	////ray_hit hit;
	//rayCastFromCamera(hit, m_cameraViewDir * maxDist, ent_all);

	//if (!hit.pCollider)
	//	return;

	//float d1 = GRAB_OBJECT_DIST;
	//float f = hit.dist;
	//float d2 = f / (1 + radius / d1);

	//float k = d2 / d1;

	////Vec3 origin = m_camera->GetWorldTransformMatrix().GetTranslation();
	//const Vec3 &dir = m_cameraViewDir;

	//Vec3 newPos = origin + dir * d1 * k;

	//entity->SetPos(newPos);
	//entity->SetScale(Vec3(k));
	//entity->GetPhysics()->AddGeometry;

	//CryLogAlways("d1 = %f\nk = %f\nd2 = %f\nf = %f\nk * r - f = %f", d1, k, d2, f, k * radius - f);
}

void Player::pickObject() {
	const float maxPickDist = 2.5;

	if (m_grabbedObject == nullptr) {
		ray_hit hit;
		rayCastFromCamera(hit, m_cameraViewDir * GRAB_OBJECT_DIST, ent_rigid | ent_sleeping_rigid);

		if (hit.pCollider) {
			IEntity* entity = gEnv->pEntitySystem->GetEntityFromPhysics(hit.pCollider);

			m_grabbedObject = entity;
			m_grabbedObject->EnablePhysics(false);
		}
	}
	else {
		//m_grabbedObject->EnablePhysics(true);
		doPerspectiveScaling(m_grabbedObject);
		m_grabbedObject = nullptr;
	}
}

IEntity* Player::rayCastFromCamera(ray_hit &hit, const Vec3 &dir, int objTypes) {
	Vec3 origin = m_camera->GetWorldTransformMatrix().GetTranslation();

	//CryLogAlways("from %f %f %f", origin.x, origin.y, origin.z);
	//CryLogAlways("dir %f %f %f", dir.x, dir.y, dir.z);
	
	const unsigned int flags = rwi_stop_at_pierceable | rwi_colltype_any;

	IPhysicalEntity* skip = m_character->GetEntity()->GetPhysics();

	gEnv->pPhysicalWorld->RayWorldIntersection(origin, dir, objTypes, flags, &hit, 1, skip);


	if (hit.pCollider) {
		/*IPersistantDebug* db = gEnv->pGameFramework->GetIPersistantDebug();
		if (hit.pCollider) {
			db->Begin("RC hit", false);
			db->AddSphere(hit.pt, 0.05f, ColorF(0, 0, 1), 4.0);
		}*/

		IPhysicalEntity* physEnt = hit.pCollider;
		IEntity* entity = gEnv->pEntitySystem->GetEntityFromPhysics(physEnt);
		return entity;
	}
	return nullptr;
}

void Player::HandleInputFlagChange(const CEnumFlags<EInputFlag> flags, const CEnumFlags<EActionActivationMode> activationMode, const EInputFlagType type)
{
	switch (type)
	{
	case EInputFlagType::Hold:
	{
		if (activationMode == eAAM_OnRelease)
		{
			m_inputFlags &= ~flags;
		}
		else
		{
			m_inputFlags |= flags;
		}
	}
	break;
	case EInputFlagType::Toggle:
	{
		if (activationMode == eAAM_OnRelease)
		{
			m_inputFlags ^= flags;
		}
	}
	break;
	}
}


static void RegisterPlayerComponent(Schematyc::IEnvRegistrar& registrar)
{
	Schematyc::CEnvRegistrationScope scope = registrar.Scope(IEntity::GetEntityScopeGUID());
	{
		Schematyc::CEnvRegistrationScope componentScope = scope.Register(SCHEMATYC_MAKE_ENV_COMPONENT(Player));
		{
		}
	}
}

CRY_STATIC_AUTO_REGISTER_FUNCTION(&RegisterPlayerComponent)