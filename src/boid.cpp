#include "boid.h"

#include <algorithm>
#include <array>
#include <random>

static constexpr auto min_speed = 250.0f;
static constexpr auto max_speed = 500.0f;

static auto rng = []() {
    auto seed_data = std::array<int, std::mt19937::state_size>();
    auto rd = std::random_device();
    std::generate_n(seed_data.data(), seed_data.size(), std::ref(rd));
    auto seed_seq = std::seed_seq(seed_data.begin(), seed_data.end());
    return std::mt19937(seed_seq);
}();
static auto brightness_dist = std::uniform_int_distribution<sf::Uint8>(128, 255);
static auto speed_dist = std::uniform_real_distribution<float>(min_speed, max_speed);

static auto clamp(const sf::Vector2f& vector, const float min, const float max)
{
    const auto length = vector.length();
    if (length == 0.0f)
        return vector;
    return vector.normalized() * std::clamp(length, min, max);
}

Boid::Boid(const sf::Vector2f& position, const sf::Angle& rotation)
    : sf::ConvexShape(4)
    , m_velocity(speed_dist(rng), rotation)
{
    setPoint(0, { 2, 0 });
    setPoint(1, { -2, -2 });
    setPoint(2, { -1, 0 });
    setPoint(3, { -2, 2 });
    setScale({ 10.0f, 10.0f });
    setPosition(position);
    setRotation(rotation);
    const auto brightness = brightness_dist(rng);
    m_color = { brightness, brightness, brightness };
    reset_color();
}

void Boid::flock(const std::vector<Boid*>& neighbors, const Gain& gain)
{
    m_acceleration = {};
    if (neighbors.empty())
        return;

    const auto for_all_neighbors = [neighbors](const auto& transform) {
        return std::accumulate(neighbors.begin(), neighbors.end(), sf::Vector2f {}, transform);
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

    m_acceleration = (alignment + cohesion + separation) / (float)neighbors.size();
    m_acceleration = clamp(m_acceleration, 0.0f, 500.0f);
}

void Boid::update(const sf::Time& dt, const sf::VideoMode& video_mode)
{
    move(dt.asSeconds() * m_velocity);
    m_velocity = clamp(m_velocity + dt.asSeconds() * m_acceleration, min_speed, max_speed);

    const auto width = (float)video_mode.size.x;
    const auto height = (float)video_mode.size.y;
    setRotation(m_velocity.angle());
    setPosition({ fmodf(getPosition().x + width, width), fmodf(getPosition().y + height, height) });
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
