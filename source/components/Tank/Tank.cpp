#include "Tank.h"

#include <UnigineVisualizer.h>
#include <UnigineGame.h>

using namespace Unigine;

REGISTER_COMPONENT(Tank);

void Tank::init()
{
    // get the wheel joints from the nodes
    if (wheel_7)
        joint_wheel_7 = checked_ptr_cast<JointWheel>(wheel_7->getObjectBody()->getJoint(0));
    if (wheel_6)
        joint_wheel_6 = checked_ptr_cast<JointWheel>(wheel_6->getObjectBody()->getJoint(0));
    if (wheel_5)
        joint_wheel_5 = checked_ptr_cast<JointWheel>(wheel_5->getObjectBody()->getJoint(0));
    if (wheel_4)
        joint_wheel_4 = checked_ptr_cast<JointWheel>(wheel_4->getObjectBody()->getJoint(0));
    if (wheel_3)
        joint_wheel_3 = checked_ptr_cast<JointWheel>(wheel_3->getObjectBody()->getJoint(0));
    if (wheel_2)
        joint_wheel_2 = checked_ptr_cast<JointWheel>(wheel_2->getObjectBody()->getJoint(0));
    if (wheel_1)
        joint_wheel_1 = checked_ptr_cast<JointWheel>(wheel_1->getObjectBody()->getJoint(0));
    if (wheel_0)
        joint_wheel_0 = checked_ptr_cast<JointWheel>(wheel_0->getObjectBody()->getJoint(0));
    // get the settings of input controls relevant to the player (camera)
    controls = Game::getPlayer()->getControls();
}

void Tank::update()
{
    // get the time it took to render the previous frame
    float deltaTime = Game::getIFps();

    current_torque = 0.0f;

    // process control inputs
    if (controls)
    {
        // set the torque and increase the current velocity if the forward button is pressed
        if (controls->getState(Controls::STATE_FORWARD) != 0)
        {
            current_torque = default_torque;
            current_velocity = Math::max(current_velocity, 0.0f);
            current_velocity += deltaTime * acceleration;
        }
        // set the torque and decrease the current velocity if the backward button is pressed
        else if (controls->getState(Controls::STATE_BACKWARD) != 0)
        {
            current_torque = default_torque;
            current_velocity = Math::min(current_velocity, 0.0f);
            current_velocity -= deltaTime * acceleration;
        }
        // exponentially decrease the current velocity when neither throttle nor brakes are applied
        else
        {
            current_velocity *= Math::exp(-deltaTime);
        }
        /*...*/
        // turn the front wheels based on the direction input
        if (controls->getState(Controls::STATE_MOVE_LEFT) != 0)
            current_turn_angle += deltaTime * turn_speed;
        else if (controls->getState(Controls::STATE_MOVE_RIGHT) != 0)
            current_turn_angle -= deltaTime * turn_speed;
        else
        {
            // get rid of the front wheel wiggle
            if (Math::abs(current_turn_angle) < 0.25f)
                current_turn_angle = 0.0f;

            // align the front wheels if there are no more direction input
            current_turn_angle -= Math::sign(current_turn_angle) * turn_speed * deltaTime;
        }

        // apply braking by maxing out the angular damping if the brake button is pressed
        if (controls->getState(Controls::STATE_USE) != 0)
        {
            joint_wheel_7->setAngularDamping(10000.0f);
            joint_wheel_6->setAngularDamping(10000.0f);
            joint_wheel_5->setAngularDamping(10000.0f);
            joint_wheel_4->setAngularDamping(10000.0f);
            joint_wheel_3->setAngularDamping(10000.0f);
            joint_wheel_2->setAngularDamping(10000.0f);
        }
        else
        {
            joint_wheel_7->setAngularDamping(0.0f);
            joint_wheel_6->setAngularDamping(0.0f);
            joint_wheel_5->setAngularDamping(0.0f);
            joint_wheel_4->setAngularDamping(0.0f);
            joint_wheel_3->setAngularDamping(0.0f);
            joint_wheel_2->setAngularDamping(0.0f);
        }
    }

    // clamp the velocity and the front wheels turn angle
    current_velocity = Math::clamp(current_velocity, -max_velocity, max_velocity);
    current_turn_angle = Math::clamp(current_turn_angle, -max_turn_angle, max_turn_angle);

    // figure out the correct turn angle for the front wheels
    float angle_0 = current_turn_angle;
    float angle_1 = current_turn_angle;
    if (Math::abs(current_turn_angle) > Math::Consts::EPS)
    {
        float radius = Tank_base / Math::tan(current_turn_angle * Math::Consts::DEG2RAD);
        float radius_0 = radius - Tank_width * 0.5f;
        float radius_1 = radius + Tank_width * 0.5f;

        angle_0 = Math::atan(Tank_base / radius_0) * Math::Consts::RAD2DEG;
        angle_1 = Math::atan(Tank_base / radius_1) * Math::Consts::RAD2DEG;
    }

    joint_wheel_0->setAxis10(Math::rotateZ(angle_1).getColumn3(0));
    joint_wheel_1->setAxis10(Math::rotateZ(angle_0).getColumn3(0));
}

// actually apply the torque and velocity to the front wheel joints
void Tank::updatePhysics()
{
    // Log::message("velocity: %f, torque: %f\n", current_velocity, current_torque);
    joint_wheel_1->setAngularVelocity(current_velocity);
    joint_wheel_0->setAngularVelocity(current_velocity);

    joint_wheel_1->setAngularTorque(current_torque);
    joint_wheel_0->setAngularTorque(current_torque);
}
