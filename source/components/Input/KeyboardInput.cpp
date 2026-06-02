#include "KeyboardInput.h"
#include <UnigineInput.h>
#include <UnigineGame.h>
#include <UnigineConsole.h>
#include "InputUtils.h"

using namespace Unigine;
using namespace Math;

REGISTER_COMPONENT(KeyboardInput)

// key bind functions
void KeyboardInput::KeyBind::set(InputInterface* input, const KeyboardKey &key_param, String description)
{
	key = Unigine::Input::KEY(key_param.key_param.get());
	default_value = (key_param.default_state != 0);

	input->binds[description] = Input::getKeyName(key);
}

void KeyboardInput::KeyBind::update()
{
	current_value = false;
	is_key_down = false;

	if (!Console::isActive() && key != -1)
	{
		current_value = Input::isKeyPressed(static_cast<Input::KEY>(key));
		is_key_down = Input::isKeyDown(static_cast<Input::KEY>(key));
	}
}

bool KeyboardInput::KeyBind::getCurrentValue() const
{
	if (default_value && !is_once_updated)
	{
		is_once_updated = true;
		return true;
	}

	return current_value;
}

// axis bind functions
void KeyboardInput::AxisBind::set(InputInterface* input, const KeyboardAxis &axis_param, String description)
{
	positive_key = Input::KEY(axis_param.positive_key_param.get());
	negative_key = Input::KEY(axis_param.negative_key_param.get());

	axis_clamp = axis_param.clamp_param.get();
	axis_inverse = axis_param.inverse_param.get();
	dead = axis_param.dead_zone_param.get();
	acceleration = axis_param.acceleration_param.get();
	gravity = axis_param.gravity_param.get();

	switch (axis_clamp)
	{
		case FULL:             current_value = 0.0f;  break;
		case POSITIVE_RANGE:   current_value = 0.0f;  break;
		case NEGATIVE_RANGE:   current_value = 0.0f;  break;
		case FULL_TO_POSITIVE: current_value = -1.0f; break;
		case FULL_TO_NEGATIVE: current_value = 1.0f;  break;
	}

	if (axis_inverse)
		current_value *= -1.0f;

	String key = Input::getKeyName(positive_key);
	key += " | ";
	key += Input::getKeyName(negative_key);

	input->binds[description] = key;
}

void KeyboardInput::AxisBind::update(float ifps)
{
	int positive = 0;
	if (!Console::isActive() && positive_key != -1)
		positive = Input::isKeyPressed(static_cast<Input::KEY>(positive_key));

	int negative = 0;
	if (!Console::isActive() && negative_key != -1)
		negative = Input::isKeyPressed(static_cast<Input::KEY>(negative_key));

	if (positive || negative)
		current_value += (positive - negative) * acceleration * ifps;
	else if (Math::abs(current_value) > dead)
	{
		int current_sign = static_cast<int>(sign(current_value));
		current_value -= current_sign * gravity * ifps;

		if (current_sign != static_cast<int>(sign(current_value)))
			current_value = 0.0f;
	}

	current_value = clamp(current_value, -1.0f, 1.0f);
}

float KeyboardInput::AxisBind::getCurrentValue() const
{
	float value = 0.0f;
	switch (axis_clamp)
	{
		case FULL:             value = current_value;                break;
		case POSITIVE_RANGE:   value = max(0.0f, current_value);     break;
		case NEGATIVE_RANGE:   value = min(0.0f, current_value);     break;
		case FULL_TO_POSITIVE: value = current_value / 2.0f + 0.5f;  break;
		case FULL_TO_NEGATIVE: value = -current_value / 2.0f - 0.5f; break;
	}

	float abs_value = Math::abs(value);
	if ((axis_clamp == FULL_TO_POSITIVE || axis_clamp == FULL_TO_NEGATIVE) &&
		(0.5f - dead < abs_value) && (abs_value < 0.5f + dead))
	{
		value = 0.5f * sign(value);
	} else if (abs_value < dead)
		value = 0.0f;

	if (axis_inverse)
	{
		if (axis_clamp == FULL)
			return -value;

		if (axis_clamp == NEGATIVE_RANGE || axis_clamp == FULL_TO_NEGATIVE)
			return -(1.0f + value);

		return 1.0f - value;
	}

	return value;
}

// car keyboard input functions
void KeyboardInput::init()
{
	axis_binds.clear();
	key_binds.clear();

#define BIND_SET(NAME, VECTOR)                                                               \
	NAME##_bind.set(this, NAME##_param, splitWords(NAME##_param.getParameter()->getTitle()));\
	VECTOR.append(&NAME##_bind);                                                             \

	// set axes
	BIND_SET(throttle, axis_binds)
	BIND_SET(yaw, axis_binds)
	BIND_SET(pitch, axis_binds)
	BIND_SET(roll, axis_binds)

	BIND_SET(camera_axis_y, axis_binds)
	BIND_SET(camera_axis_x, axis_binds)
	BIND_SET(camera_zoom_in, axis_binds)
	BIND_SET(camera_zoom_out, axis_binds)

	// set keys
	BIND_SET(control_mode, key_binds)
	BIND_SET(change_camera_mode, key_binds)
	BIND_SET(next_camera, key_binds)
	BIND_SET(camera_zoom_reset, key_binds)
	BIND_SET(fpv_auto_balance, key_binds)
	BIND_SET(reset_camera_rot, key_binds)
	BIND_SET(restore, key_binds)
#undef BIND_SET
}

void KeyboardInput::update()
{
	float ifps = Game::getIFps();

	for (auto ab : axis_binds)
		ab->update(ifps);

	for (auto kb : key_binds)
		kb->update();
}

void KeyboardInput::shutdown()
{

}

bool KeyboardInput::isStartLanding() const
{
	return throttle_bind.getCurrentValue() <= -0.85
		   && Math::compare(pitch_bind.getCurrentValue(), 0.f)
		   && Math::compare(roll_bind.getCurrentValue(), 0.f);
}

bool KeyboardInput::isStartFlight() const
{
	//	return throttle_bind.getCurrentValue() <= -0.50 && yaw_bind.getCurrentValue() >= 0.50
	//		&& roll_bind.getCurrentValue() <= -0.50 && pitch_bind.getCurrentValue() <= -0.50;
	return throttle_bind.getCurrentValue() >= 0.85
		   && Math::compare(pitch_bind.getCurrentValue(), 0.f)
		   && Math::compare(roll_bind.getCurrentValue(), 0.f);
}

