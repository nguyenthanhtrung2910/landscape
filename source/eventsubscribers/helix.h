#pragma once

#include <json/json.h>
#include <boost/asio.hpp>
#include <queue>

#include "AppWorldLogic.h"
#include "eventsubscribers/eventsubscriber.h"

class Helix : public EventSubscriber
{
public:
    Helix(float radius, float slope, float height, int length, int time_offset, boost::asio::io_context *io);
    explicit Helix(const char *json_path, int time_offset, boost::asio::io_context *io);
    void start(AppWorldLogic *worldLogic);
    void stop();

private:
    void load(const Json::Value &json);
    Unigine::Math::Vec3 calculateEventPos(float t) const;
    Unigine::Math::quat calculateEventQuat(float t) const;
    boost::asio::awaitable<void> timer(AppWorldLogic *worldLogic);

private:
    float m_radius{};
    float m_slope{};
    float m_height{};
    int m_length{};

    std::chrono::seconds m_deltaT{};
    boost::asio::io_context *m_io{};
    boost::asio::steady_timer m_timer;
};
