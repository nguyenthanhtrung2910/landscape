#include "Drone.h"

#include <iostream>

#include <UnigineGame.h>
#include <UnigineMathLibMat3.h>

using namespace Unigine;
using namespace Math;

REGISTER_COMPONENT(Drone)

void Drone::init()
{
	m_worldLogic = dynamic_cast<AppWorldLogic *>(Engine::get()->getWorldLogic(0));
	input = ComponentSystem::get()->getComponentInWorld<InputManager>();
	if (!input)
		Log::error("LinearModel::init(): there is no InputManager in the world\n");
	if (!anchor.get())
		Log::error("LinearModel::init(): there is no anchor node\n");
}

void Drone::shutdown()
{
}

void Drone::update()
{
	float ifps{Game::getIFps()};
	if (m_waiting)
	{
		if (m_worldLogic->hasEvents())
		{
			m_targetFlightEvent = m_worldLogic->popEvent();
			m_prePosEvent = node->getWorldPosition();
			m_preRotEvent = node->getWorldRotation();
			m_time = 0.0f;
			m_waiting = false;
		}
	}
	if (!m_waiting)
	{
		Mat4 nextTranform{anchor.get()->getWorldTransform() * Mat4{m_targetFlightEvent.rotation, m_targetFlightEvent.position}};
		Vec3 nextPos{nextTranform.getTranslate()};
		quat nextQuat{nextTranform.getRotate()};
		float len = (float)length(nextPos - m_prePosEvent);
		m_time += velocity / len * ifps;
		if (m_time >= 1.0f)
		{
			node->setWorldTransform(nextTranform);
			m_waiting = true;
			// drone_camera_point_to->setWorldDirection(
			// 	vec3((home->getWorldPosition() - drone_camera_point_to->getWorldPosition()).normalize()),
			// 	vec3_up);
			return;
		}
		else
		{
			node->setWorldPosition(lerp(m_prePosEvent, nextPos, m_time));
			node->setWorldRotation(slerp(m_preRotEvent, nextQuat, m_time));
			// drone_camera_point_to->setWorldDirection(
			// 	vec3((home->getWorldPosition() - drone_camera_point_to->getWorldPosition()).normalize()),
			// 	vec3_up);
			return;
		}
	}
	float throttle_input{input->getThrottleAxis()};
	float yaw_input{input->getYawAxis()};
	float pitch_input{input->getPitchAxis()};
	float roll_input{input->getRollAxis()};

	m_prevRot = m_currentRot;
	m_prevAngles = m_currentAngles;
	m_prevLinearVelocity = m_linearVelocity;

	m_currentAngles.x = -Utils::rerange(-max_pitch_angle.get(), max_pitch_angle.get(), -1.f, 1.f, pitch_input);
	m_currentAngles.y = Utils::rerange(-max_roll_angle.get(), max_roll_angle.get(), -1.f, 1.f, roll_input);
	m_currentAngles.z = m_prevAngles.z - Utils::rerange(-max_angular_velocity.get(), max_angular_velocity.get(), -1.f, 1.f, yaw_input) * ifps;
	// rotation
	m_currentRot = quat(0, 0, m_currentAngles.z) * quat(m_currentAngles.x, m_currentAngles.y, 0.f);
	m_currentRot = Math::slerp(m_prevRot, m_currentRot, 1.0f - Math::exp(-max_angular_velocity.get() * ifps));
	node->setWorldRotation(m_currentRot);
	// translation
	m_linearVelocity.y = Utils::rerange<float>(-max_speed.get(), max_speed.get(), -max_pitch_angle.get(), max_pitch_angle.get(), -m_currentAngles.x);
	m_linearVelocity.x = Utils::rerange<float>(-max_speed.get(), max_speed.get(), -max_roll_angle.get(), max_roll_angle.get(), m_currentAngles.y);
	m_linearVelocity.z = throttle_input < 0 ? -Math::lerp(0.f, max_descent_speed.get(), Math::abs(throttle_input))
											: Math::lerp(0.f, max_ascent_speed.get(), throttle_input);
	m_linearVelocity = Math::lerp(m_prevLinearVelocity, m_linearVelocity, 1.0f - Math::exp(-max_speed.get() * ifps));
	node->worldTranslate((Math::rotateZ(m_currentAngles.z) * Math::translate(m_linearVelocity * ifps)).getTranslate());
}
