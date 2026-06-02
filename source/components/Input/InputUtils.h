#pragma once
#include <UnigineComponentSystem.h>
#include <string>
#include <typeinfo>

const char * getAxisClampName(int axis_clamp);

const char * getGamepadButtonName(int button);

const char * getGamepadAxisName(int axis);

const char * getGamepadStickName(int stick);

// LooksLikeThat -> Looks Like That
const char * splitWords(const char * str);