#include "Boid.hpp"

#include <algorithm>
#include <array>
#include <functional>
#include <numeric>
#include <random>

std::mt19937& rng()
{
    thread_local auto rng = []() {
        auto seed_data = std::array<std::random_device::result_type, std::mt19937::state_size>();
        auto rd = std::random_device();
        std::generate_n(seed_data.data(), seed_data.size(), std::ref(rd));
        auto seed_seq = std::seed_seq(seed_data.begin(), seed_data.end());
        return std::mt19937(seed_seq);
    }();
    return rng;
}

namespace {
auto brightness_dist = std::uniform_int_distribution<uint16_t>(128, 255);
auto speed_dist = std::uniform_real_distribution(250.f, 500.f);

auto clamp(const sf::Vector2f& vector, const float min, const float max)
{
    const auto length = vector.length();
    if (length == 0)
        return vector;
    return vector.normalized() * std::clamp(length, min, max);
}
}

Boid::Boid(const sf::Vector2f& position, const sf::Angle& rotation)
    : sf::ConvexShape(4)
    , m_velocity(speed_dist(rng()), rotation)
{
    setPoint(0, { 2, 0 });
    setPoint(1, { -2, -2 });
    setPoint(2, { -1, 0 });
    setPoint(3, { -2, 2 });
    setScale({ 10, 10 });
    setPosition(position);
    setRotation(rotation);
    const auto brightness = uint8_t(brightness_dist(rng()));
    m_color = { brightness, brightness, brightness };
    reset_color();
}

void Boid::flock(const std::vector<Boid*>& neighbors, const Gain& gain, const sf::Vector2u& window_size)
{
    if (neighbors.empty())
        return;

    const auto for_all_neighbors = [neighbors](const auto& transform) {
        return std::accumulate(neighbors.cbegin(), neighbors.cend(), sf::Vector2f(), transform);
    };

    const auto alignment = for_all_neighbors([this](const sf::Vector2f& sum, const Boid* boid) {
                               return sum + boid->get_velocity() - get_velocity();
                           })
        * gain.alignment;
    const auto cohesion = for_all_neighbors([this](const sf::Vector2f& sum, const Boid* boid) {
                              return sum + boid->getPosition() - getPosition();
                          })
        * gain.cohesion;
    const auto separation = for_all_neighbors([this](const sf::Vector2f& sum, const Boid* boid) {
                                const auto diff = getPosition() - boid->getPosition();
                                return sum + diff / diff.lengthSq();
                            })
        * gain.separation;

    constexpr auto lookahead_distance = 100.f;
    const auto control_point = getPosition() + sf::Vector2f(lookahead_distance, getRotation());

    constexpr auto inset = 50.f;
    const auto distance_from_left = std::max(inset - control_point.x, 0.f);
    const auto distance_from_right = std::max(control_point.x - float(window_size.x) + inset, 0.f);
    const auto distance_from_top = std::max(inset - control_point.y, 0.f);
    const auto distance_from_bottom = std::max(control_point.y - float(window_size.y) + inset, 0.f);
    const auto avoidance
        = 25.f * sf::Vector2f(distance_from_left - distance_from_right, distance_from_top - distance_from_bottom);

    m_acceleration = (alignment + cohesion + separation) / float(neighbors.size());
    m_acceleration = clamp(m_acceleration, 0, 800);
    m_acceleration += avoidance;
}

void Boid::update(const sf::Time& dt)
{
    move(dt.asSeconds() * m_velocity);
    m_velocity = clamp(m_velocity + dt.asSeconds() * m_acceleration, speed_dist.min(), speed_dist.max());
    setRotation(m_velocity.angle());
}

auto Boid::can_see(const Boid& neighbor, const float perception_radius, const sf::Angle& perception_angle) const -> bool
{
    if (this == &neighbor)
        return false;
    const auto to_neighbor = neighbor.getPosition() - getPosition();
    const auto is_within_radius = to_neighbor.lengthSq() < perception_radius * perception_radius;
    if (is_within_radius && to_neighbor != sf::Vector2f()) {
        const auto angle_to_neighbor = m_velocity.angleTo(to_neighbor);
        if (angle_to_neighbor < perception_angle && angle_to_neighbor > -perception_angle)
            return true;
    }
    return false;
}
