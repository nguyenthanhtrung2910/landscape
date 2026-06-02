#pragma once

#include <array>
#include <utility>
#include <iostream>

#include <UnigineMathLib.h>

struct FlightEvent
{
    Unigine::Math::Vec3 position;
    Unigine::Math::quat rotation;

    FlightEvent() = default;

    FlightEvent(
        const Unigine::Math::Vec3 &position,
        const Unigine::Math::quat &rotation)
        : position(position),
          rotation(rotation)
    {
    }
    FlightEvent(const std::array<float, 6> &data)
        : position{data[0], data[1], data[2]},
          rotation{Unigine::Math::rotationFromDir(
              Unigine::Math::vec3{data[3], data[4], data[5]},
              Unigine::Math::vec3_up)}
    {
    }
};

inline std::ostream &operator<<(std::ostream &os, const FlightEvent &event)
{
    os << "FlightEvent {\n"
       << "  position = ("
       << event.position.x << ", "
       << event.position.y << ", "
       << event.position.z << ")\n"
       << "  rotation = ("
       << event.rotation.x << ", "
       << event.rotation.y << ", "
       << event.rotation.z << ", "
       << event.rotation.w << ")\n"
       << "}";

    return os;
}

class EventSubscriber
{
public:
    virtual ~EventSubscriber() = default;
};
