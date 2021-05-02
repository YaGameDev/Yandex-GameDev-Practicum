// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryEntitySystem/IEntitySystem.h>
#include <DefaultComponents/Input/InputComponent.h>
#include <DefaultComponents/Physics/RigidBodyComponent.h>
#include <DefaultComponents/Physics/CharacterControllerComponent.h>
#include <DefaultComponents/Cameras/CameraComponent.h>

class Player final : public IEntityComponent
{
	const float GRAB_OBJECT_DIST = 2;

	Cry::DefaultComponents::CInputComponent* m_input = nullptr;
	Cry::DefaultComponents::CCharacterControllerComponent* m_character = nullptr;
	Cry::DefaultComponents::CCameraComponent* m_camera = nullptr;

	bool m_debug = false;

	Matrix34 m_bodyOrientation = IDENTITY;
	Vec3 m_cameraViewDir = FORWARD_DIRECTION;
	Vec2 m_mouseDelta = ZERO;

	std::vector<Vec3> m_grabbedObjectPoints;
	
	enum class EInputFlag : uint8
	{
		MoveLeft = 1 << 0,
		MoveRight = 1 << 1,
		MoveForward = 1 << 2,
		MoveBack = 1 << 3
	};

	enum class EInputFlagType
	{
		Hold = 0,
		Toggle
	};

	CEnumFlags<EInputFlag> m_inputFlags;

	IEntity* m_grabbedObject = nullptr;


	void updateMovement(float delta);
	void updateCamera(float delta);
	void lockLocalPoints();
	void updateGrabbedObject(float delta);
	void doPerspectiveScaling();
	void pickObject();
	IEntity* rayCastFromCamera(ray_hit &hit, const Vec3 &dir, int objTypes);

public:
	Player() = default;
	virtual ~Player() = default;

	static void ReflectType(Schematyc::CTypeDesc<Player>& desc)
	{
		desc.SetGUID("{41316132-8A1E-4073-B0CD-A242FD3D2E90}"_cry_guid);
		desc.SetEditorCategory("Game");
		desc.SetLabel("Player");
		desc.SetDescription("Player spawn point");
		desc.SetComponentFlags({ IEntityComponent::EFlags::Transform, IEntityComponent::EFlags::Socket, IEntityComponent::EFlags::Attach });
	}

protected:
	virtual Cry::Entity::EventFlags GetEventMask() const override;
	virtual void Initialize() override;
	virtual void ProcessEvent(const SEntityEvent& event) override;
	void HandleInputFlagChange(const CEnumFlags<EInputFlag> flags, const CEnumFlags<EActionActivationMode> activationMode, const EInputFlagType type = EInputFlagType::Hold);
};

