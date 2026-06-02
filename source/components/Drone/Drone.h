#pragma once

#include <UnigineComponentSystem.h>

#include "AppWorldLogic.h"
#include "components/Input/InputManager.h"
#include "components/utils.h"

class Drone : public Unigine::ComponentBase
{
public:
	COMPONENT_DEFINE(Drone, Unigine::ComponentBase);

	COMPONENT_INIT(init, INIT_ORDER::DRONE);
	COMPONENT_UPDATE(update);
	COMPONENT_SHUTDOWN(shutdown);
	PROP_PARAM(Node, anchor);
	PROP_PARAM(Float, velocity, 10.0f);
	PROP_PARAM(Float, max_speed, 5.0f);
	PROP_PARAM(Float, max_descent_speed, 1.5f);
	PROP_PARAM(Float, max_ascent_speed, 1.5f);
	PROP_PARAM(Float, max_pitch_angle, 10.0f);
	PROP_PARAM(Float, max_roll_angle, 10.0f);
	PROP_PARAM(Float, max_angular_velocity, 30.0f);

public:
	Unigine::Math::Vec3 getPosition() { return node->getWorldPosition(); }

private:
	void init();
	void shutdown();
	void update();

private:
	Unigine::PlayerDummyPtr drone_camera_point_to{};
	bool m_waiting{true};
	float m_time{};

	AppWorldLogic *m_worldLogic{};
	Unigine::Math::Vec3 m_prePosEvent{};
	Unigine::Math::quat m_preRotEvent{};
	FlightEvent m_targetFlightEvent{};

	InputManager *input{};
	Unigine::Math::vec3 m_currentAngles;
	Unigine::Math::vec3 m_prevAngles;
	Unigine::Math::quat m_currentRot;
	Unigine::Math::quat m_prevRot;
	Unigine::Math::vec3 m_linearVelocity;
	Unigine::Math::vec3 m_prevLinearVelocity;
};
