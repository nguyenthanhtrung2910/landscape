#pragma once

#include <queue>

#include <json/json.h>
#include <boost/asio.hpp>

#include "eventsubscribers/eventsubscriber.h"
#include "AppWorldLogic.h"

class UDPReceiver : public EventSubscriber
{
public:
    UDPReceiver(boost::asio::io_context *io);
    void start(AppWorldLogic *worldLogic);
    void stop();

private:
    void load(const Json::Value &json);
    boost::asio::awaitable<void> receiver(AppWorldLogic *worldLogic);

private:
    boost::asio::io_context *m_io{};
    boost::asio::ip::udp::socket m_socket;
};
