#pragma once

#include <SFML/System/Vector2.hpp>

#include <algorithm>
#include <cmath>

inline constexpr auto pi = 3.1415926f;
inline constexpr auto to_radians = pi / 180.0f;
inline constexpr auto to_degrees = 180.0f / pi;

inline auto dot(const sf::Vector2f& lhs, const sf::Vector2f& rhs) { return lhs.x * rhs.x + lhs.y * rhs.y; }

inline auto length2(const sf::Vector2f& vector) noexcept { return dot(vector, vector); }

inline auto length(const sf::Vector2f& vector) noexcept { return std::sqrt(length2(vector)); }

inline auto clamp(const sf::Vector2f& vector, const float min, const float max)
{
    const auto l = length(vector);
    if (l == 0.0f)
        return vector;
    return vector * std::clamp(l, min, max) / l;
}
