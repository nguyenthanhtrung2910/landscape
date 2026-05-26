//
// client.cpp
// ~~~~~~~~~~
//
// Copyright (c) 2003-2025 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <array>
#include <cmath>
#include <iostream>
#include <string>
#include <vector>

#include <boost/asio.hpp>

using boost::asio::ip::udp;

namespace
{
    constexpr float pi = 3.14159265358979323846f;

    struct Vec3
    {
        Vec3() = default;
        Vec3(float x_, float y_, float z_)
            : x(x_),
              y(y_),
              z(z_)
        {
        }

        float x{};
        float y{};
        float z{};
    };

    constexpr Vec3 Vec3_zero{};

    class Helix
    {
    public:
        Helix(float radius, float slope, float height, int length)
            : m_radius(radius),
              m_slope(slope),
              m_height(height),
              m_length(length)
        {
        }

        int length() const
        {
            return m_length;
        }

        std::array<float, 6> calculateEvent(int index) const
        {
            const float t = static_cast<float>(index) * 8.0f * pi / static_cast<float>(m_length);
            const Vec3 position = calculateEventPos(t);
            const Vec3 direction = calculateEventDirection(t);

            return {
                position.x,
                position.y,
                position.z,
                direction.x,
                direction.y,
                direction.z,
            };
        }

    private:
        Vec3 calculateEventPos(float t) const
        {
            const float b = m_radius / m_slope;
            if (t >= 0.0f && t <= 4.0f * pi)
                return Vec3(
                    m_radius * std::sin(t),
                    m_radius * std::cos(t),
                    m_height + b * t);

            if (t > 4.0f * pi && t < 8.0f * pi)
                return Vec3(
                    m_radius * std::sin(t),
                    m_radius * std::cos(t),
                    m_height + b * (8.0f * pi - t));

            return Vec3_zero;
        }

        Vec3 calculateEventDirection(float t) const
        {
            const float b = m_radius / m_slope;
            if (t >= 0.0f && t <= 4.0f * pi)
                return Vec3(
                    m_radius * std::cos(t),
                    -m_radius * std::sin(t),
                    b);

            if (t > 4.0f * pi && t < 8.0f * pi)
                return Vec3(
                    m_radius * std::cos(t),
                    -m_radius * std::sin(t),
                    -b);

            return Vec3_zero;
        }

    private:
        float m_radius{};
        float m_slope{};
        float m_height{};
        int m_length{};
    };

    int parseInt(const char *value, int fallback)
    {
        try
        {
            return std::stoi(value);
        }
        catch (...)
        {
            return fallback;
        }
    }

    float parseFloat(const char *value, float fallback)
    {
        try
        {
            return std::stof(value);
        }
        catch (...)
        {
            return fallback;
        }
    }

}

int main(int argc, char *argv[])
{
    try
    {
        if (argc != 2 && argc != 6)
        {
            std::cerr << "Usage: client <host> [radius slope height length]" << std::endl;
            return 1;
        }

        const float radius = argc == 6 ? parseFloat(argv[2], 10.0f) : 10.0f;
        const float slope = argc == 6 ? parseFloat(argv[3], 20.0f) : 20.0f;
        const float height = argc == 6 ? parseFloat(argv[4], 3.0f) : 3.0f;
        const int length = argc == 6 ? parseInt(argv[5], 500) : 500;

        if (slope == 0.0f || length <= 0)
        {
            std::cerr << "Invalid helix parameters" << std::endl;
            return 1;
        }

        constexpr std::size_t max_udp_payload_size = 65507;
        if (static_cast<std::size_t>(length) * 6 * sizeof(float) > max_udp_payload_size)
        {
            std::cerr << "Payload is too large for one UDP datagram" << std::endl;
            return 1;
        }

        boost::asio::io_context io_context;

        udp::resolver resolver(io_context);
        udp::endpoint receiver_endpoint =
            *resolver.resolve(udp::v4(), argv[1], "8080").begin();

        udp::socket socket(io_context);
        socket.open(udp::v4());

        Helix helix{radius, slope, height, length};
        std::vector<float> send_buf;
        send_buf.reserve(static_cast<std::size_t>(helix.length()) * 6);

        for (int i = 0; i < helix.length(); ++i)
        {
            const std::array<float, 6> event = helix.calculateEvent(i);
            send_buf.insert(send_buf.end(), event.begin(), event.end());
        }

        socket.send_to(boost::asio::buffer(send_buf), receiver_endpoint);
    }
    catch (std::exception &e)
    {
        std::cerr << e.what() << std::endl;
    }

    return 0;
}
