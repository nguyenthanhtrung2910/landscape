#pragma once

#include <UnigineComponentSystem.h>

#include <AppWorldLogic.h>
#include <Utils.h>

class Drone : public Unigine::ComponentBase
{
public:
	COMPONENT_DEFINE(Drone, Unigine::ComponentBase);

	COMPONENT_INIT(init, INIT_ORDER::DRONE);
	COMPONENT_UPDATE(update);
	COMPONENT_SHUTDOWN(shutdown);
	PROP_PARAM(Float, velocity, 20.4f);
	PROP_PARAM(Node, cameraPointTo);
	PROP_PARAM(Node, cameraForward);
	PROP_PARAM(Node, tank);
	Unigine::Math::Vec3 getPosition() { return node->getWorldPosition(); }

private:
	void init();
	void shutdown();
	void update();

private:
	Unigine::PlayerDummyPtr drone_camera_forward{};
	Unigine::PlayerDummyPtr drone_camera_point_to{};
	Unigine::PlayerPersecutorPtr camera_global{};
	bool m_start{false};
	bool m_waiting{true};
	float m_time{};

	AppWorldLogic *m_worldLogic{};
	Unigine::Math::Vec3 m_currentPos{};
	Unigine::Math::quat m_currentQuat{};
	FlightEvent m_targetFlightEvent{};
};
