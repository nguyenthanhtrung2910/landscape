#include "helix.h"

#include <fstream>
#include <iostream>

#include <UnigineEngine.h>

using namespace Unigine;
using namespace Math;

Helix::Helix(float radius, float slope, float height, int length, int time_offset, boost::asio::io_context *io)
    : m_radius(radius),
      m_slope(slope),
      m_height(height),
      m_length(length),
      m_deltaT(time_offset),
      m_io(io),
      m_timer(*io)
{
}

Helix::Helix(const char *json_path, int time_offset, boost::asio::io_context *io)
    : m_deltaT(time_offset),
      m_io(io),
      m_timer(*io)
{
    std::ifstream file(json_path);
    if (!file.is_open())
        return;
    Json::Value json;
    Json::CharReaderBuilder builder;
    if (!Json::parseFromStream(builder, file, &json, nullptr))
        return;
    if (!json.isObject())
        return;
    load(json);
}

void Helix::start(AppWorldLogic *worldLogic)
{
    boost::asio::co_spawn(*m_io, timer(worldLogic), boost::asio::detached);
}

void Helix::stop()
{
    m_timer.cancel();
}

void Helix::load(const Json::Value &json)
{
    m_radius = json.get("radius", m_radius).asFloat();
    m_slope = json.get("slope", m_slope).asFloat();
    m_height = json.get("height", m_height).asFloat();
    m_length = json.get("length", m_length).asInt();
}

Vec3 Helix::calculateEventPos(float t) const
{
    float b{m_radius / m_slope};
    if (t >= 0.0f && t <= 4 * Math::Consts::PI)
        return Vec3(
            m_radius * sin(t),
            m_radius * cos(t),
            m_height + b * t);
    if (t > 4 * Consts::PI && t < 8 * Consts::PI)
        return Vec3(
            m_radius * sin(t),
            m_radius * cos(t),
            m_height + b * (8 * Consts::PI - t));
    return Vec3_zero;
}

quat Helix::calculateEventQuat(float t) const
{
    float b{m_radius / m_slope};
    vec3 direction{};
    if (t >= 0.0f && t <= 4 * Consts::PI)
        direction = vec3(
            m_radius * cos(t),
            -m_radius * sin(t),
            b);
    else if (t > 4 * Consts::PI && t < 8 * Consts::PI)
        direction = vec3(
            m_radius * cos(t),
            -m_radius * sin(t),
            -b);
    return rotationFromDir(direction, vec3_up);
}

boost::asio::awaitable<void> Helix::timer(AppWorldLogic *worldLogic)
{
    auto nextTick = std::chrono::steady_clock::now();
    while (true)
    {
        if (Unigine::Engine::get()->isQuit())
            co_return;
        nextTick += m_deltaT;
        m_timer.expires_at(nextTick);
        // queue many events at once can be long I/O operation
        // consider waiting for this opreation on other thread
        for (int i = 0; i < m_length; ++i)
        {
            worldLogic->enqueueEvent(FlightEvent{calculateEventPos(i * 8 * Math::Consts::PI / m_length),
                                                calculateEventQuat(i * 8 * Math::Consts::PI / m_length)});
        }
        co_await m_timer.async_wait(boost::asio::use_awaitable);
    }
}
