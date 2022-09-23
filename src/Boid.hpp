#pragma once

#include <SFML/Graphics.hpp>

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

    Boid(const sf::Vector2f& position, const sf::Angle& rotation);
    auto get_velocity() const -> const auto& { return m_velocity; }

    void flock(const std::vector<Boid*>& neighbors, const Gain& gain);
    void update(const sf::Time& dt, const sf::Vector2u& size);
    void select() { setFillColor(sf::Color::Red); }
    void highlight() { setFillColor(sf::Color::Yellow); }
    void reset_color() { setFillColor(m_color); }
    auto can_see(const Boid& neighbor, const float perception_radius, const sf::Angle& perception_angle) const -> bool;
};
