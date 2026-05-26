#pragma once

#include <UnigineVector.h>
#include <UnigineMathLib.h>

#ifdef __GNUC__
#define __UNIGINE_FUNCTION__ __PRETTY_FUNCTION__
#elif defined(_MSC_VER)
#define __UNIGINE_FUNCTION__ __FUNCTION__
#endif

#define PROP_SWITCH_ENUM(NAME, ...) \
	enum class NAME { __VA_ARGS__ }; \
	static const char* NAME##_STR = #__VA_ARGS__;

#define CHECK_COMPONENT_VALUE(value, text) \
if (!value) { Log::error("%s: %s %d(%s)\n", __UNIGINE_FUNCTION__, text, node->getID(), node->getName()); }

enum INIT_ORDER
{
	WEATHER = -20,
	INPUT = -10,
	DRONE = 0,
	GAME = 10,
	UI = 20,
};

namespace Utils
{

// lerp from [u0:u1] to [v0:v1]
template <typename T>
UNIGINE_INLINE T rerange(T v0, T v1, T u0, T u1, float k)
{
	return k * (v1 - v0) / (u1 - u0);
}

}
