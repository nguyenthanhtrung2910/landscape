#pragma once

#include <vector>

#include <UnigineComponentSystem.h>
#include <UniginePhysics.h>
#include <UniginePlayers.h>
#include <UnigineControls.h>
#include <UnigineGame.h>
#include <UnigineMathLib.h>

class Tank : public Unigine::ComponentBase
{
public:
	// declare constructor and destructor for our class and define a property name
	COMPONENT_DEFINE(Tank, ComponentBase)
	// declare methods to be called at the corresponding stages of the execution sequence
	COMPONENT_INIT(init);
	COMPONENT_UPDATE(update);
	COMPONENT_UPDATE_PHYSICS(updatePhysics);

	// driving parameters
	PROP_PARAM(Float, acceleration, 50.0f);
	PROP_PARAM(Float, max_velocity, 90.0f);
	PROP_PARAM(Float, default_torque, 5.0f);
	PROP_PARAM(Float, turn_speed, 50.0f);
	PROP_PARAM(Float, Tank_base, 3.0f);
	PROP_PARAM(Float, Tank_width, 2.0f);

	// wheel joints
	PROP_PARAM(Node, wheel_7);
	PROP_PARAM(Node, wheel_6);
	PROP_PARAM(Node, wheel_5);
	PROP_PARAM(Node, wheel_4);
	PROP_PARAM(Node, wheel_3);
	PROP_PARAM(Node, wheel_2);
	PROP_PARAM(Node, wheel_1);
	PROP_PARAM(Node, wheel_0);

protected:
	void init();
	void update();
	void updatePhysics();

private:
	// wheel joints
	Unigine::JointWheelPtr joint_wheel_7;
	Unigine::JointWheelPtr joint_wheel_6;
	Unigine::JointWheelPtr joint_wheel_5;
	Unigine::JointWheelPtr joint_wheel_4;
	Unigine::JointWheelPtr joint_wheel_3;
	Unigine::JointWheelPtr joint_wheel_2;
	Unigine::JointWheelPtr joint_wheel_1;
	Unigine::JointWheelPtr joint_wheel_0;

	// settings of input controls
	Unigine::ControlsPtr controls;

	// current driving parameters
	float current_velocity = 0.0f;
	float current_torque = 0.0f;
	float current_turn_angle = 0.0f;
	float max_turn_angle = 30.0f;
};