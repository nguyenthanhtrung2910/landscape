#include "udpreceiver.h"

#include <array>
#include <fstream>
#include <iostream>

#include <UnigineEngine.h>

using namespace Unigine;
using namespace Math;

UDPReceiver::UDPReceiver(boost::asio::io_context *io)
    : m_io(io),
      m_socket(*io, boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), 8080))
{
}

void UDPReceiver::start(AppWorldLogic *worldLogic)
{
    boost::asio::co_spawn(*m_io, receiver(worldLogic), boost::asio::detached);
}

void UDPReceiver::stop()
{
    m_socket.close();
}

boost::asio::awaitable<void> UDPReceiver::receiver(AppWorldLogic *worldLogic)
{
    boost::asio::ip::udp::endpoint remote_endpoint;
    constexpr std::size_t values_per_event = 6; // 3 for position, 3 for direction
    constexpr std::size_t max_udp_payload_size = 65507;
    constexpr std::size_t max_float_count = max_udp_payload_size / sizeof(float);
    std::array<float, max_float_count> data;

    while (true)
    {
        const std::size_t bytes_received = co_await m_socket.async_receive_from(
            boost::asio::buffer(data),
            remote_endpoint,
            boost::asio::use_awaitable);

        if (bytes_received % sizeof(float) != 0)
        {
            std::cerr << "Received malformed UDP packet: byte count is not float-aligned" << std::endl;
            continue;
        }

        const std::size_t float_count = bytes_received / sizeof(float);
        if (float_count % values_per_event != 0)
        {
            std::cerr << "Received malformed UDP packet: event data is not divisible by 6" << std::endl;
            continue;
        }

        for (std::size_t i = 0; i < float_count; i += values_per_event)
        {
            worldLogic->enqueueEvent(FlightEvent{
                Vec3{data[i], data[i + 1], data[i + 2]},
                rotationFromDir({data[i + 3], data[i + 4], data[i + 5]}, vec3_up),
            });
        }
    }
}
