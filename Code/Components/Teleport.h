#pragma once

#include <CryEntitySystem/IEntitySystem.h>
#include <DefaultComponents/Physics/BoxPrimitiveComponent.h>

class Teleport final : public IEntityComponent
{
	bool m_playerInside = false;
	IEntity* m_gateway = nullptr;
	IEntity* m_player = nullptr;
	Cry::DefaultComponents::CBoxPrimitiveComponent *m_collider = nullptr;

	const string TP_LINK_NAME = "TP";
	float scale = 1.f;

	//Vec3 m_size;

public:
	Teleport() = default;
	virtual ~Teleport() = default;

	static void ReflectType(Schematyc::CTypeDesc<Teleport>& desc)
	{
		desc.SetGUID("{6A7E1851-A17F-4CEC-9760-C61C196248DF}"_cry_guid);
		desc.SetEditorCategory("Game");
		desc.SetLabel("Teleport");
		desc.SetDescription("Teleportation unit");
		desc.SetComponentFlags({ IEntityComponent::EFlags::Socket, IEntityComponent::EFlags::Attach });

		desc.AddMember(&Teleport::scale, 'scal', "Scale", "Scale", "Portal Scaling", 1.f);
	}

protected:
	virtual void Initialize() override;

	virtual Cry::Entity::EventFlags GetEventMask() const override;
	virtual void ProcessEvent(const SEntityEvent& event) override;
};

