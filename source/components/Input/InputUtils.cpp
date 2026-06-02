#include "InputUtils.h"
#include <UnigineInterface.h>
#include <UnigineInput.h>
#include "InputInterface.h"

using namespace Unigine;

const char * getAxisClampName(int axis_clamp)
{
	switch (axis_clamp)
	{
	case InputInterface::FULL:             return "FULL";
	case InputInterface::FULL_TO_NEGATIVE: return "FULL TO NEGATIVE";
	case InputInterface::FULL_TO_POSITIVE: return "FULL TO POSITIVE";
	case InputInterface::NEGATIVE_RANGE:   return "NEGATIVE_RANGE";
	case InputInterface::POSITIVE_RANGE:   return "POSITIVE RANGE";
	default: return "";
	}
}

const char * getGamepadButtonName(int button)
{
	switch (button)
	{
	case Input::GAMEPAD_BUTTON_A:              return "A";
	case Input::GAMEPAD_BUTTON_B:              return "B";
	case Input::GAMEPAD_BUTTON_X:              return "X";
	case Input::GAMEPAD_BUTTON_Y:              return "Y";
	case Input::GAMEPAD_BUTTON_BACK:           return "BACK";
	case Input::GAMEPAD_BUTTON_START:          return "START";
	case Input::GAMEPAD_BUTTON_DPAD_UP:        return "DPAD UP";
	case Input::GAMEPAD_BUTTON_DPAD_DOWN:      return "DPAD DOWN";
	case Input::GAMEPAD_BUTTON_DPAD_LEFT:      return "DPAD LEFT";
	case Input::GAMEPAD_BUTTON_DPAD_RIGHT:     return "DPAD RIGHT";
	case Input::GAMEPAD_BUTTON_THUMB_LEFT:     return "THUMB LEFT";
	case Input::GAMEPAD_BUTTON_THUMB_RIGHT:    return "THUMB RIGHT";
	case Input::GAMEPAD_BUTTON_SHOULDER_LEFT:  return "LB";
	case Input::GAMEPAD_BUTTON_SHOULDER_RIGHT: return "RB";
	default: return "NONE";
	}
}

const char * getGamepadAxisName(int axis)
{
	switch (axis)
	{
	case InputInterface::AXIS_LEFT_X:   return "LEFT STICK HORIZONTAL";
	case InputInterface::AXIS_LEFT_Y:   return "LEFT STICK VERTICAL";
	case InputInterface::AXIS_RIGHT_X:  return "RIGHT STICK HORIZONTAL";
	case InputInterface::AXIS_RIGHT_Y:  return "RIGHT STICK VERTICAL";
	case InputInterface::LEFT_TRIGGER:  return "LEFT TRIGGER";
	case InputInterface::RIGHT_TRIGGER: return "RIGHT TRIGGER";
	case InputInterface::TRIGGERS: return "LR-TRIGGERS";
	default: return "NONE";
	}
}

const char * getGamepadStickName(int stick)
{
	switch (stick)
	{
	case InputInterface::LEFT_STICK:   return "LEFT STICK";
	case InputInterface::RIGHT_STICK:   return "RIGHT STICK";
	default: return "NONE";
	}
}

const char* splitWords(const char* str)
{
	String ans;

	int i = 0;
	while (str[i] != '\0')
	{
		if (str[i] >= 'A' && str[i] <= 'Z' && (str[i+1] < 'A' || str[i+1] > 'Z'))
		{
			ans += ' ';
		}
		ans += str[i];

		i++;
	}

	ans = ans.trim(" ");

	return ans.getRaw();
}