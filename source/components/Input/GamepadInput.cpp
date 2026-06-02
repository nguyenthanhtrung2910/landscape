#include "GamepadInput.h"
#include <UnigineInput.h>
#include <UnigineGame.h>

using namespace Unigine;
using namespace Math;

REGISTER_COMPONENT(GamepadInput)

// key bind functions
void GamepadInput::KeyBind::set(InputInterface* input, const GamepadKey &key_param, Unigine::String description)
{
	key = key_param.key_param.get();

	input->binds[description] = getGamepadButtonName(key);
}

void GamepadInput::KeyBind::update(const InputGamePadPtr &gamepad)
{
	auto prev_value = current_value;
	current_value = false;
	
	if (key == -1 || !gamepad || !gamepad->isAvailable())
		return;

	if (Input::GAMEPAD_BUTTON_A <= key && key <= Input::GAMEPAD_BUTTON_SHOULDER_RIGHT)
		current_value = (gamepad->isButtonPressed(static_cast<Input::GAMEPAD_BUTTON >(key)) != 0);

	is_button_down = current_value && (current_value != prev_value);
}

bool GamepadInput::KeyBind::getCurrentValue() const
{
	return current_value;
}

// axis bind functions
void GamepadInput::AxisBind::set(InputInterface* input, const GamepadAxis &axis_param, Unigine::String description)
{
	axis = axis_param.axis_param.get();
	axis_clamp = axis_param.clamp_param.get();
	axis_inverse = axis_param.inverse_param.get();
	dead = axis_param.dead_zone_param.get();

	use_fake_axis = (axis_param.use_fake_axis_param.get() != 0);
	positive_key = axis_param.positive_key_param.get();
	negative_key = axis_param.negative_key_param.get();

	sensitivity = axis_param.sensitivity_param.get();

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

	String key = getGamepadAxisName(axis);
	if (use_fake_axis)
	{
		key = getGamepadButtonName(positive_key);
		key += " | ";
		key += getGamepadButtonName(negative_key);
	}

	input->binds[description] = key;
}

void GamepadInput::AxisBind::update(float ifps, const InputGamePadPtr &gamepad)
{
	if (!gamepad || !gamepad->isAvailable())
	{
		current_value = 0.0f;
		return;
	}

	if (use_fake_axis)
	{
		int positive = 0;

		if (positive_key != -1 && Input::GAMEPAD_BUTTON_A <= positive_key && positive_key <= Input::GAMEPAD_BUTTON_SHOULDER_RIGHT)
			positive = gamepad->isButtonPressed(static_cast<Input::GAMEPAD_BUTTON>(positive_key));

		int negative = 0;

		if (negative_key != -1 && Input::GAMEPAD_BUTTON_A <= negative_key && negative_key <= Input::GAMEPAD_BUTTON_SHOULDER_RIGHT)
			negative = gamepad->isButtonPressed(static_cast<Input::GAMEPAD_BUTTON>(negative_key));

		if (positive || negative)
			current_value += (positive - negative) * sensitivity * ifps;
		else if (Math::abs(current_value) > dead)
		{
			int current_sign = static_cast<int>(sign(current_value));
			current_value -= current_sign * sensitivity * ifps;

			if (current_sign != static_cast<int>(sign(current_value)))
				current_value = 0.0f;
		}

//		current_value = clamp(current_value, -1.0f, 1.0f);
	}
	else
	{
		auto get_triggers_value = [](float left, float right)
		{
			if (left > right) return -left;
			else return right;
		};

		current_value = 0.0f;

		switch (axis)
		{
		case AXIS_LEFT_X:   current_value = gamepad->getAxesLeft().x;   break;
		case AXIS_LEFT_Y:   current_value = gamepad->getAxesLeft().y;   break;
		case AXIS_RIGHT_X:  current_value = gamepad->getAxesRight().x;  break;
		case AXIS_RIGHT_Y:  current_value = gamepad->getAxesRight().y;  break;
		case LEFT_TRIGGER:  current_value = gamepad->getTriggerLeft();  break;
		case RIGHT_TRIGGER: current_value = gamepad->getTriggerRight(); break;
		case TRIGGERS:      current_value = get_triggers_value(gamepad->getTriggerLeft(), gamepad->getTriggerRight()); break;
		default: break;
		}
	}

	current_value = clamp(current_value, -1.0f, 1.0f);
}

float GamepadInput::AxisBind::getCurrentValue() const
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
	}
	else if (abs_value < dead)
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

void GamepadInput::init()
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

	if (Input::getNumGamePads() > 0)
		gamepad = Input::getGamePad(0);

	if (gamepad && gamepad->isAvailable())
		Log::message("Gamepad %s created successfully.\n", gamepad->getName());
	else
		Log::warning("Gamepad is not available.\n");

}

void GamepadInput::update()
{
	if (!gamepad || !gamepad->isAvailable())
	{
		if (Input::getNumGamePads() > 0)
			gamepad = Input::getGamePad(0);

		if (gamepad && gamepad->isAvailable())
			Log::message("Gamepad %s created successfully.\n", gamepad->getName());

		return;
	}

	float ifps = Game::getIFps();

	// update axis
	for (auto &a : axis_binds)
		a->update(ifps, gamepad);

	// update keys
	for (auto &k : key_binds)
		k->update(gamepad);
}

void GamepadInput::shutdown()
{
}

void GamepadInput::update_axis_bind(AxisBind & axis_bind, float ifps)
{
	axis_bind.update(ifps, gamepad);
}

void GamepadInput::update_key_bind(KeyBind & key_bind)
{
	key_bind.update(gamepad);
}

bool GamepadInput::isStartLanding() const
{
	return throttle_bind.getCurrentValue() <= -0.85
		&& Math::compare(pitch_bind.getCurrentValue(), 0.f)
		&& Math::compare(roll_bind.getCurrentValue(), 0.f);
}

bool GamepadInput::isStartFlight() const
{
//	return throttle_bind.getCurrentValue() <= -0.50 && yaw_bind.getCurrentValue() >= 0.50
//		&& roll_bind.getCurrentValue() <= -0.50 && pitch_bind.getCurrentValue() <= -0.50;
	return throttle_bind.getCurrentValue() >= 0.85
		&& Math::compare(pitch_bind.getCurrentValue(), 0.f)
		&& Math::compare(roll_bind.getCurrentValue(), 0.f);
}

void GamepadInput::GamepadStick::update(float ifps, const Unigine::InputGamePadPtr & gamepad)
{
	if (!gamepad || !gamepad->isAvailable())
	{
		current_value = vec2();
		return;
	}

	{
		current_value = vec2();

		switch (type)
		{
		case LEFT_STICK:  current_value = gamepad->getAxesLeft();   break;
		case RIGHT_STICK: current_value = gamepad->getAxesRight();  break;
		}
		current_length = current_value.length();
	}
}
