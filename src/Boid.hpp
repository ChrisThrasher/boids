#pragma once

#include <SFML/Graphics/ConvexShape.hpp>
#include <SFML/System/Time.hpp>
#include <random>

[[nodiscard]] std::mt19937& rng();

class Boid : public sf::ConvexShape {
    sf::Vector2f m_velocity;
    sf::Vector2f m_acceleration;
    sf::Color m_color;

public:
    struct Gain {
        float alignment;
        float cohesion;
        float separation;
    };

    Boid(sf::Vector2f position, sf::Angle rotation);
    auto get_velocity() const { return m_velocity; }

    void flock(const std::vector<Boid*>& neighbors, const Gain& gain, const sf::Vector2u& window_size);
    void update(const sf::Time& dt);
    void select() { setFillColor(sf::Color::Red); }
    void highlight() { setFillColor(sf::Color::Yellow); }
    void reset_color() { setFillColor(m_color); }
    auto can_see(const Boid& neighbor, float perception_radius, sf::Angle perception_angle) const -> bool;
};
