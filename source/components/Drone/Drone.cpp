#include "Drone.h"

#include <iostream>

#include <UnigineGame.h>
#include <UnigineMathLibMat3.h>

using namespace Unigine;
using namespace Math;

REGISTER_COMPONENT(Drone)

void Drone::init()
{
	drone_camera_point_to = checked_ptr_cast<PlayerDummy>(cameraPointTo.get());
	drone_camera_forward = checked_ptr_cast<PlayerDummy>(cameraForward.get());
	m_currentPos = node->getPosition();
	m_currentQuat = node->getRotation();
	m_worldLogic = dynamic_cast<AppWorldLogic *>(Engine::get()->getWorldLogic(0));
}

void Drone::shutdown()
{
}

void Drone::update()
{
	if (Input::isKeyDown(Input::KEY_DIGIT_1))
		Game::setPlayer(drone_camera_forward);
	else if (Input::isKeyDown(Input::KEY_DIGIT_2))
		Game::setPlayer(drone_camera_point_to);
	else if (Input::isKeyDown(Input::KEY_C))
	{
		m_start = true;
	}
	else if (Input::isKeyDown(Input::KEY_V))
		m_start = false;
	if (!m_start)
		return;
	if (m_waiting)
	{
		if (m_worldLogic->hasEvents())
		{
			m_targetFlightEvent = m_worldLogic->popEvent();
			m_waiting = false;
		}
		else
			return;
	}
	Vec3 *nextPos = &m_targetFlightEvent.position;
	quat *nextQuat = &m_targetFlightEvent.rotation;
	float len = (float)length(*nextPos - m_currentPos);
	m_time += velocity / len * Game::getIFps();
	if (m_time >= 1.0f)
	{
		node->setPosition(*nextPos);
		node->setRotation(*nextQuat);
		m_currentPos = *nextPos;
		m_currentQuat = *nextQuat;
		m_time = 0.0f;
		if (m_worldLogic->hasEvents())
			m_targetFlightEvent = m_worldLogic->popEvent();
		else
			m_waiting = true;
	}
	else
	{
		node->setPosition(lerp(m_currentPos, *nextPos, m_time));
		node->setRotation(slerp(m_currentQuat, *nextQuat, m_time));
	}
	drone_camera_point_to->setWorldDirection(
		vec3((tank->getWorldPosition() - drone_camera_point_to->getWorldPosition()).normalize()),
		vec3_up);
}
