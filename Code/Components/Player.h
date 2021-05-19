#pragma once

#include <CryEntitySystem/IEntitySystem.h>
#include <DefaultComponents/Input/InputComponent.h>
#include <DefaultComponents/Physics/RigidBodyComponent.h>
#include <DefaultComponents/Physics/CharacterControllerComponent.h>
#include <DefaultComponents/Cameras/CameraComponent.h>

class Player final : public IEntityComponent
{
	const float DEFAULT_GRAB_OBJECT_DIST = 0.2;
	float GRAB_OBJECT_DIST = DEFAULT_GRAB_OBJECT_DIST;

	const float CHARACHTER_Z = 1.2f;
	const float CHARACHTER_RADIUS = 0.8;
	const float CHARACHTER_HEIGHT = 1.8;
	const float CHARACHTER_MOVESPEED = 50.f;

	Cry::DefaultComponents::CInputComponent* m_input = nullptr;
	Cry::DefaultComponents::CCharacterControllerComponent* m_character = nullptr;
	Cry::DefaultComponents::CCameraComponent* m_camera = nullptr;

	float m_start_scale = 1.f;
	float m_scale = 1.f;
	bool m_debug = false;
	bool m_shouldTeleport = false;
	Vec3 m_teleportVelocity = ZERO;

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
	void applyCharacterScale(float scale);

public:
	void teleport(Vec3 to, float zAng, float setScale);
	float getScale();

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

		desc.AddMember(&Player::m_start_scale, 'scal', "Scale", "Scale", "Start Scaling", 1.f);
	}

protected:
	virtual Cry::Entity::EventFlags GetEventMask() const override;
	virtual void Initialize() override;
	virtual void ProcessEvent(const SEntityEvent& event) override;
	void HandleInputFlagChange(const CEnumFlags<EInputFlag> flags, const CEnumFlags<EActionActivationMode> activationMode, const EInputFlagType type = EInputFlagType::Hold);
};

