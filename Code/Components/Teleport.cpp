#include "StdAfx.h"
#include "Teleport.h"

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

#include <CryGame/IGameFramework.h>

#include "Components/Player.h"

bool debug = false;

void Teleport::Initialize()
{
	m_collider = m_pEntity->GetOrCreateComponent<Cry::DefaultComponents::CBoxPrimitiveComponent>();
	
}

Cry::Entity::EventFlags Teleport::GetEventMask() const
{
	return Cry::Entity::EEvent::Update | Cry::Entity::EEvent::GameplayStarted;
}

bool isPointInside(const Vec3& point, const Vec3 &selfPos, const Vec3 &size, float zAng) {
	Vec3 newPoint = point - selfPos;
	newPoint = newPoint.GetRotated(Vec3(0, 0, 1), -zAng);

	AABB aabb(-size, size);

	/*if (debug) {
		IPersistantDebug* db = gEnv->pGameFramework->GetIPersistantDebug();

		if (aabb.IsContainPoint(newPoint)) {
			db->AddSphere(selfPos + newPoint, 0.2, ColorF(0, 0, 1), 1);
		}
		else {
			db->AddSphere(selfPos + newPoint, 0.2, ColorF(1, 0, 0), 1);
		}
	}*/

	return aabb.IsContainPoint(newPoint);
}

void Teleport::ProcessEvent(const SEntityEvent& event)
{
	switch (event.event) {
		case Cry::Entity::EEvent::GameplayStarted:
		{
			CryLogAlways("Gameplay started!");
			m_player = gEnv->pEntitySystem->FindEntityByName("Player");

			IEntityLink* link = m_pEntity->GetEntityLinks();
			while (link) {
				if (link->name == TP_LINK_NAME)
				{
					m_gateway = gEnv->pEntitySystem->GetEntity(link->entityId);
					break;
				}
				link = link->next;
			}

			if (!m_player)
				CryLogAlways("Player entity not found");

			if (!m_gateway)
				CryLogAlways("Gateway entity not found");
		}
		break;

		case Cry::Entity::EEvent::Update:
		{
			//if (gEnv->IsEditor())
			//	return;

			if (m_player == nullptr || m_gateway == nullptr)
				return;

			const float delta = event.fParam[0];

			Vec3 pos = m_pEntity->GetWorldPos();
			Vec3 size = m_collider->m_size;
			Vec3 offs = Vec3(0, 0, size.z);

			if (debug) {
				IPersistantDebug* db = gEnv->pGameFramework->GetIPersistantDebug();
				db->Begin(m_pEntity->GetGuid().ToString(), true);
				db->AddDirection(pos + offs, 1, m_pEntity->GetForwardDir(), ColorF(1, 1, 1), 1);
			}

			Player* playerComp = m_player->GetComponent<Player>();
			assert(playerComp && "No player component found!");

			Vec3 playerPos = m_player->GetWorldPos();
			
			float alpha1 = m_pEntity->GetWorldRotation().GetRotZ();
			float alpha2 = m_gateway->GetWorldRotation().GetRotZ();

			//bool playerInsideNow = aabb.IsContainPoint(playerPos + Vec3(0, 0, playerComp->getScale()));

			bool playerInsideNow = isPointInside(playerPos, pos, size, alpha1);

			if (playerInsideNow) {
				m_playerInside = true;
			}

			else if (m_playerInside) {
				m_playerInside = false;

				Teleport* teleportComp = m_gateway->GetComponent<Teleport>();
				assert(teleportComp && "Teleport component found!");
	
				float totalScale = teleportComp->scale / scale;

				float angDiff = alpha2 - alpha1;
				Vec3 diff = (playerPos - pos) * totalScale;
				diff = diff.GetRotated(Vec3(0, 0, 1), angDiff);

				Vec3 newPlayerPos = m_gateway->GetWorldPos() + diff;

				playerComp->teleport(newPlayerPos, angDiff, totalScale);

				CryLogAlways("TP!");
			}
		}
		break;
	}
}


static void RegisterTeleportComponent(Schematyc::IEnvRegistrar& registrar)
{
	Schematyc::CEnvRegistrationScope scope = registrar.Scope(IEntity::GetEntityScopeGUID());
	{
		Schematyc::CEnvRegistrationScope componentScope = scope.Register(SCHEMATYC_MAKE_ENV_COMPONENT(Teleport));
		{
		}
	}
}

CRY_STATIC_AUTO_REGISTER_FUNCTION(&RegisterTeleportComponent)
