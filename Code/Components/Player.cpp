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
#include <DefaultComponents/Geometry/StaticMeshComponent.h>
#include <DefaultComponents/Input/InputComponent.h>

void Player::Initialize()
{
	m_character = m_pEntity->GetOrCreateComponent<Cry::DefaultComponents::CCharacterControllerComponent>();
	
	m_camera = m_pEntity->GetOrCreateComponent<Cry::DefaultComponents::CCameraComponent>();
	m_input = m_pEntity->GetOrCreateComponent<Cry::DefaultComponents::CInputComponent>();


	m_character->SetTransformMatrix(Matrix34::Create(Vec3(1.f), IDENTITY, Vec3(0, 0, CHARACHTER_Z * m_scale)));

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


	m_input->RegisterAction("player", "exit", [this](int activationMode, float value) {
		if (activationMode == eAAM_OnPress)
			gEnv->pConsole->ExecuteString("quit");
	});
	m_input->BindAction("player", "exit", eAID_KeyboardMouse, EKeyId::eKI_Escape);

	m_input->RegisterAction("player", "toggle_debug", [this](int activationMode, float value) {
		if (activationMode == eAAM_OnPress)
			m_debug ^= 1;
	});
	m_input->BindAction("player", "toggle_debug", eAID_KeyboardMouse, EKeyId::eKI_Tab);

	m_input->RegisterAction("player", "jump", [this](int activationMode, float value) {
		if (activationMode == eAAM_OnPress)
		{
		}
	});
	m_input->BindAction("player", "jump", eAID_KeyboardMouse, EKeyId::eKI_Space);
}

Cry::Entity::EventFlags Player::GetEventMask() const
{
	return Cry::Entity::EEvent::Update | Cry::Entity::EEvent::GameplayStarted;// | Cry::Entity::EEvent::PrePhysicsUpdate;
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

		case Cry::Entity::EEvent::GameplayStarted:
		{
			m_camera->SetTransformMatrix(IDENTITY);
			m_scale = m_start_scale;
			//CryLogAlways("PLAYER GAMEPLAY STARTED!");
			applyCharacterScale(1.f);
		}
		break;
	}
}


void Player::updateMovement(float delta)
{
	if (m_debug) {
		IPersistantDebug* db = gEnv->pGameFramework->GetIPersistantDebug();
		db->Begin("DEBUG_TEXT", true);
		db->AddText(0, 0, 4, ColorF(), 1, "on ground %d", m_character->IsOnGround());
	}
	//Vec3 pos = m_character->GetTransformMatrix().GetTranslation();
	//CryLogAlways("pos tr %f %f %f", pos.x, pos.y, pos.z);
	//CryLogAlways("pos %f %f %f", pos.x, pos.y, pos.z);

	if (m_shouldTeleport) {
		m_shouldTeleport = false;
		m_character->AddVelocity(m_teleportVelocity);
	}

	if (!m_character->IsOnGround())
		return;

	Vec3 velocity = ZERO;
	const float moveSpeed = CHARACHTER_MOVESPEED * m_scale;

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
	localTransform.AddTranslation(Vec3(0, 0, cameraHeight * m_scale));

	m_camera->SetTransformMatrix(localTransform);

	m_cameraViewDir = camOrientation * FORWARD_DIRECTION;
}

void Player::lockLocalPoints() {
	IPhysicalEntity* physEnt = m_grabbedObject->GetPhysics();

	pe_status_pos spos;
	m_grabbedObject->GetPhysics()->GetStatus(&spos);
	physEnt->GetStatus(&spos);

	std::vector<Vec3>& points = m_grabbedObjectPoints;
	IGeometry* geom = spos.pGeom;

	Cry::DefaultComponents::CStaticMeshComponent *mesh = m_grabbedObject->GetComponent<Cry::DefaultComponents::CStaticMeshComponent>();

	Matrix34 localTransform = mesh ? mesh->GetTransformMatrix() : IDENTITY;

	if (geom->GetType() == GEOM_TRIMESH) {
		points.clear();

		const mesh_data* mesh = (mesh_data*) geom->GetData();

		for (int i = 0; i < mesh->nVertices; i++) {
			points.push_back(localTransform * mesh->pVertices[i]);
		}
	}
	
	else {
		const int SAMPLES = 4;

		points.clear();
		points.reserve(8 + SAMPLES * SAMPLES * 6);

		primitives::box box;
		geom->GetBBox(&box);

		//CryLogAlways("BOX POSITION %f %f %f", box.center.x, box.center.y, box.center.z);

		for (int i = 0; i < 8; i++) {
			float sx = (float)((i & 1) * 2 - 1);
			float sy = (float)((i >> 1 & 1) * 2 - 1);
			float sz = (float)((i >> 2 & 1) * 2 - 1);

			//CryLogAlways("i = %d %d %d %d", i, sx, sy, sz);
			points.push_back(Vec3(sx, sy, sz).CompMul(box.size));
		}

		std::vector<std::array<Vec3, 3>> faces = {
			{points[0], points[1], points[2]},
			{points[0], points[1], points[4]},
			{points[0], points[2], points[4]},

			{points[1], points[3], points[5]},
			{points[2], points[3], points[6]},
			{points[4], points[5], points[6]},
		};


		for (const auto& face : faces) {
			Vec3 diff_i = (face[1] - face[0]);
			Vec3 diff_j = (face[2] - face[0]);

			for (int i = 1; i < SAMPLES + 1; i++) {
				for (int j = 1; j < SAMPLES + 1; j++) {
					float fi = i / (float)(SAMPLES + 1);
					float fj = j / (float)(SAMPLES + 1);

					points.push_back(face[0] + diff_i * fi + diff_j * fj);
				}
			}
		}
	}
}

void Player::updateGrabbedObject(float delta)
{
	if (!m_grabbedObject)
		return;

	Vec3 origin = m_camera->GetWorldTransformMatrix().GetTranslation();
	Vec3 newPos = origin + m_cameraViewDir * GRAB_OBJECT_DIST;

	m_grabbedObject->SetPos(newPos);

	if (m_debug) {
		IPersistantDebug* db = gEnv->pGameFramework->GetIPersistantDebug();
		db->Begin("obb", true);
		for (const Vec3& point : m_grabbedObjectPoints) {
			db->AddSphere(m_grabbedObject->GetWorldTM() * point, 0.01f, ColorF(1, 0, 1), 1.0);
		}
	}
}

void Player::doPerspectiveScaling() {
	const float maxDist = 150;
	const float wallMargin = 0.05f;

	std::vector<Vec3> points = m_grabbedObjectPoints;
	for (auto& point : points) {
		point = m_grabbedObject->GetWorldTM() * point;
	}

	Vec3 origin = m_camera->GetWorldTransformMatrix().GetTranslation();
	ray_hit hit;
	
	float minQ = 0;
	float minDist = 2 * maxDist;
	int minRay = -1;

	for (size_t i = 0; i < points.size(); i++) {
		Vec3 dir = points[i] - origin;
		float ln = dir.len();
		dir = dir / ln * maxDist;

		//if (ln > GRAB_OBJECT_DIST) 
		{
			rayCastFromCamera(hit, dir, ent_all);

			if (!hit.pCollider)
				continue;
			
			float hit_dist = std::max(hit.dist - wallMargin, 0.f);

			float q = (hit_dist - ln) / ln;

			if (minRay == -1 || q < minQ) {
				minDist = hit_dist;
				minQ = q;
				minRay = i;
			}

			if (m_debug)
			{
				IPersistantDebug* db = gEnv->pGameFramework->GetIPersistantDebug();
				db->Begin("scale", false);
				db->AddLine(points[i], hit.pt, ColorF(0.5, 0.5, 0.5), 40.f);
				db->AddSphere(Vec3::CreateLerp(points[i], hit.pt, 0.5), 0.05, ColorF(0, 0, 0), 40);
			}
		}
	}
	
	if (minRay == -1) {
		Vec3 newPos = origin + m_cameraViewDir * (GRAB_OBJECT_DIST + wallMargin);

		m_grabbedObject->SetPos(newPos);
		return;
	}

	Vec3 old = points[minRay] - origin;
	float old_ln = old.len();
	Vec3 dir = old / old_ln;

	float oldP = old.dot(m_cameraViewDir);
	float newP = oldP * minDist / old_ln;

	float d1 = GRAB_OBJECT_DIST;
	float f = newP;
	float d2 = f / (1 + (oldP - d1) / d1);

	float k = d2 / d1;

	Vec3 newPos = origin + m_cameraViewDir * d1 * k;

	if (m_debug)
	{
		IPersistantDebug* db = gEnv->pGameFramework->GetIPersistantDebug();
		db->Begin("scale", false);
		db->AddSphere(origin + m_cameraViewDir * oldP, 0.05f, ColorF(1, 0, 0), 40.0);
		db->AddSphere(origin + m_cameraViewDir * newP, 0.05f, ColorF(1, 0, 1), 40.0);
		db->AddSphere(points[minRay], 0.05f, ColorF(1, 1, 1), 40.0);

		db->AddSphere(origin + dir * minDist, 0.05f, ColorF(0, 0, 0), 40.0);
	}

	m_grabbedObject->SetPos(newPos);
	m_grabbedObject->SetScale(k * m_grabbedObject->GetScale());
}

void Player::pickObject() {
	const float volumeThreshold = 40.f;
	const float pickRange = 150.f;

	if (m_grabbedObject == nullptr) {
		ray_hit hit;
		rayCastFromCamera(hit, m_cameraViewDir * pickRange, ent_rigid | ent_sleeping_rigid);

		if (hit.pCollider) {
			IEntity* entity = gEnv->pEntitySystem->GetEntityFromPhysics(hit.pCollider);
			
			AABB aabb;
			entity->GetWorldBounds(aabb);

			//CryLogAlways("vol %f", aabb.GetVolume());

			m_grabbedObject = entity;
		
			m_grabbedObject->EnablePhysics(false);
			lockLocalPoints();

			//float dist = hit.dist;
			float dist = (m_grabbedObject->GetPos() - m_camera->GetWorldTransformMatrix().GetTranslation()).len();
			float k = GRAB_OBJECT_DIST / dist;

			m_grabbedObject->SetScale(m_grabbedObject->GetScale() * k);
		}
	}

	else {
		m_grabbedObject->EnablePhysics(true);

		IPhysicalEntity* physEnt = m_grabbedObject->GetPhysics();
		pe_action_reset reset = pe_action_reset();
		physEnt->Action(&reset);
		pe_action_awake awake = pe_action_awake();
		physEnt->Action(&awake);

		doPerspectiveScaling();

		m_grabbedObject = nullptr;
	}
}

IEntity* Player::rayCastFromCamera(ray_hit &hit, const Vec3 &dir, int objTypes) {
	Vec3 origin = m_camera->GetWorldTransformMatrix().GetTranslation();
	
	const unsigned int flags = rwi_stop_at_pierceable | rwi_colltype_any;

	IPhysicalEntity* skip = m_character->GetEntity()->GetPhysics();

	gEnv->pPhysicalWorld->RayWorldIntersection(origin, dir, objTypes, flags, &hit, 1, skip);


	if (hit.pCollider) {
		if (m_debug) {
			IPersistantDebug* db = gEnv->pGameFramework->GetIPersistantDebug();
			db->Begin("RC hit", false);
			db->AddSphere(hit.pt, 0.05f, ColorF(0, 0, 1), 40.0);
		}

		IPhysicalEntity* physEnt = hit.pCollider;
		IEntity* entity = gEnv->pEntitySystem->GetEntityFromPhysics(physEnt);
		return entity;
	}
	return nullptr;
}

void Player::applyCharacterScale(float scale)
{
	m_scale *= scale;
	m_character->SetTransformMatrix(Matrix34::Create(Vec3(1.f), IDENTITY, Vec3(0, 0, CHARACHTER_Z * m_scale)));

	auto& params = m_character->GetPhysicsParameters();

	params.m_height = CHARACHTER_HEIGHT * m_scale;
	params.m_radius = CHARACHTER_RADIUS * m_scale;

	m_character->Physicalize();

	//GRAB_OBJECT_DIST = DEFAULT_GRAB_OBJECT_DIST * m_scale;
	//if (m_grabbedObject) {
	//	m_grabbedObject->SetScale(m_grabbedObject->GetScale() * scale);
	//}
}

void Player::teleport(Vec3 to, float zAng, float scale)
{
	m_shouldTeleport = true;
	m_teleportVelocity = m_character->GetVelocity().GetRotated(Vec3(0, 0, 1), zAng) * scale;

	m_pEntity->SetPos(to);
	applyCharacterScale(scale);

	Matrix34 rot = IDENTITY;
	rot.SetRotationZ(zAng);

	m_camera->SetTransformMatrix(rot * m_camera->GetTransformMatrix());

	if (m_grabbedObject) {
		m_grabbedObject->SetRotation(Quat(rot) * m_grabbedObject->GetRotation());
	}
}

float Player::getScale()
{
	return m_scale;
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