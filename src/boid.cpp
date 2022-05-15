#include "boid.h"
#include "vector_utils.h"

#include <random>

static constexpr auto min_speed = 250.0f;
static constexpr auto max_speed = 500.0f;

static auto rng = std::mt19937(std::random_device()());
static auto brightness_dist = std::uniform_int_distribution<sf::Uint8>(128, 255);
static auto velocity_dist = std::uniform_real_distribution<float>(min_speed, max_speed);

Boid::Boid(const sf::Vector2f& position, const float rotation)
    : sf::ConvexShape(4)
{
    setPoint(0, { 2, 0 });
    setPoint(1, { -2, -2 });
    setPoint(2, { -1, 0 });
    setPoint(3, { -2, 2 });
    setScale(10.0f, 10.0f);
    setPosition(position);
    setRotation(rotation);
    const auto brightness = brightness_dist(rng);
    m_color = { brightness, brightness, brightness };
    reset_color();

    m_velocity = velocity_dist(rng)
        * sf::Vector2f { std::cos(getRotation() * to_radians), std::sin(getRotation() * to_radians) };
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
                                return sum + diff / length2(diff);
                            })
        * gain.separation;

    m_acceleration = (alignment + cohesion + separation) / (float)neighbors.size();
    m_acceleration = clamp(m_acceleration, 0.0f, 500.0f);
}

void Boid::update(const float dt, const sf::VideoMode& video_mode)
{
    move(dt * m_velocity);
    m_velocity = clamp(m_velocity + dt * m_acceleration, min_speed, max_speed);

    const auto width = (float)video_mode.width;
    const auto height = (float)video_mode.height;
    setRotation(std::atan2(m_velocity.y, m_velocity.x) * to_degrees);
    setPosition(fmodf(getPosition().x + width, width), fmodf(getPosition().y + height, height));
}

auto Boid::can_see(const Boid& neighbor, const float perception_radius, const float perception_angle) const -> bool
{
    if (this == &neighbor)
        return false;
    const auto to_neighbor = neighbor.getPosition() - getPosition();
    const auto is_within_radius = length2(to_neighbor) < perception_radius * perception_radius;
    if (is_within_radius) {
        const auto is_within_view
            = dot(to_neighbor, m_velocity) / (length(to_neighbor) * length(m_velocity)) > std::cos(perception_angle);
        if (is_within_view)
            return true;
    }
    return false;
}
