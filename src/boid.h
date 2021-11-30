#pragma once

#include <SFML/Graphics.hpp>

class Boid : public sf::ConvexShape {
    sf::Vector2f m_velocity {};
    sf::Vector2f m_acceleration {};
    sf::Color m_color {};

public:
    struct Gain {
        float alignment;
        float cohesion;
        float separation;
    };

    Boid(const sf::Vector2f& position, const float rotation);
    auto GetVelocity() const -> const auto& { return m_velocity; }

    void Flock(const std::vector<Boid*>& neighbors, const Gain& gain);
    void Update(const float dt, const sf::VideoMode& video_mode);
    void Select() { setFillColor(sf::Color::Red); }
    void Highlight() { setFillColor(sf::Color::Yellow); }
    void ResetColor() { setFillColor(m_color); }
    auto CanSee(const Boid& neighbor, const float perception_radius, const float perception_angle) const -> bool;
};
