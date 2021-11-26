#pragma once

#include <SFML/System/Vector2.hpp>

#include <algorithm>
#include <cmath>

inline constexpr auto pi = 3.1415926f;
inline constexpr auto to_radians = pi / 180.0f;
inline constexpr auto to_degrees = 180.0f / pi;

inline auto Dot(const sf::Vector2f& lhs, const sf::Vector2f& rhs) { return lhs.x * rhs.x + lhs.y * rhs.y; }

inline auto Length2(const sf::Vector2f& vector) noexcept { return Dot(vector, vector); }

inline auto Length(const sf::Vector2f& vector) noexcept { return std::sqrt(Length2(vector)); }

inline auto Clamp(const sf::Vector2f& vector, const float min, const float max)
{
    const auto length = Length(vector);
    if (length == 0.0f)
        return vector;
    return vector * std::clamp(length, min, max) / length;
}
