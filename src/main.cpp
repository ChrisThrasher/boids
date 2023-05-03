#include "Boid.hpp"

#include <SFML/Graphics.hpp>

#include <algorithm>
#include <array>
#include <iomanip>
#include <numeric>
#include <random>
#include <sstream>
#include <vector>

using namespace sf::Literals;

namespace {
const auto video_mode = sf::VideoMode({ 1280, 720 });

auto make_boids(const size_t num_boids)
{
    static auto x_position_dist = std::uniform_real_distribution(0.f, float(video_mode.size.x));
    static auto y_position_dist = std::uniform_real_distribution(0.f, float(video_mode.size.y));
    static auto rotation_dist = std::uniform_real_distribution(0.f, 360.f);

    auto boids = std::vector<Boid>();
    boids.reserve(num_boids);
    for (size_t i = 0; i < num_boids; ++i)
        boids.emplace_back(sf::Vector2f(x_position_dist(rng), y_position_dist(rng)), sf::degrees(rotation_dist(rng)));
    return boids;
}
}

int main(int argc, char* argv[])
{
    auto num_boids = 250ull;
    if (argc > 1)
        num_boids = std::stoull(argv[1]);
    auto boids = make_boids(num_boids);
    auto* selected_boid = &boids.front();
    selected_boid->select();

    auto gain = Boid::Gain { 4e1f, 4e2f, 2e6f };
    auto perception_radius = 100.f;
    auto perception_angle = 135_deg;

    auto clock = sf::Clock();
    auto font = sf::Font();
    if (!font.loadFromFile(FONT_PATH / std::filesystem::path("font.ttf")))
        throw std::runtime_error("Failed to load font");

    auto text = sf::Text(font, "", 24);
    text.setFillColor(sf::Color::White);
    text.setOutlineThickness(2);
    text.setOutlineColor(sf::Color::Black);
    text.setPosition({ 10, 5 });

    auto window = sf::RenderWindow(video_mode, std::to_string(num_boids) + " Boids");
    window.setFramerateLimit(60);

    const auto content_view = window.getView();
    auto overlay_view = window.getView();

    enum class Control { ALIGNMENT, COHESION, SEPARATION, RADIUS, ANGLE } control = Control::ALIGNMENT;

    while (window.isOpen()) {
        for (auto event = sf::Event(); window.pollEvent(event);) {
            switch (event.type) {
            case sf::Event::Closed:
                window.close();
                break;
            case sf::Event::Resized:
                overlay_view
                    = sf::View(sf::FloatRect({}, sf::Vector2f(sf::Vector2u(event.size.width, event.size.height))));
                break;
            case sf::Event::KeyPressed:
                switch (event.key.code) {
                case sf::Keyboard::Escape:
                    window.close();
                    break;
                case sf::Keyboard::Space:
                    boids = make_boids(num_boids);
                    selected_boid = &boids.front();
                    selected_boid->select();
                    break;
                case sf::Keyboard::A:
                    control = Control::ALIGNMENT;
                    break;
                case sf::Keyboard::C:
                    control = Control::COHESION;
                    break;
                case sf::Keyboard::S:
                    control = Control::SEPARATION;
                    break;
                case sf::Keyboard::R:
                    control = Control::RADIUS;
                    break;
                case sf::Keyboard::G:
                    control = Control::ANGLE;
                    break;
                case sf::Keyboard::Up:
                    switch (control) {
                    case Control::ALIGNMENT:
                        gain.alignment = std::min(gain.alignment * 2.f, 1e16f);
                        break;
                    case Control::COHESION:
                        gain.cohesion = std::min(gain.cohesion * 2.f, 1e16f);
                        break;
                    case Control::SEPARATION:
                        gain.separation = std::min(gain.separation * 2.f, 1e16f);
                        break;
                    case Control::RADIUS:
                        perception_radius += 5.f;
                        break;
                    case Control::ANGLE:
                        perception_angle = std::min(perception_angle + 5_deg, 180_deg);
                        break;
                    }
                    break;
                case sf::Keyboard::Down:
                    switch (control) {
                    case Control::ALIGNMENT:
                        gain.alignment = std::max(gain.alignment / 2.f, 1.f);
                        break;
                    case Control::COHESION:
                        gain.cohesion = std::max(gain.cohesion / 2.f, 1.f);
                        break;
                    case Control::SEPARATION:
                        gain.separation = std::max(gain.separation / 2.f, 1.f);
                        break;
                    case Control::RADIUS:
                        perception_radius = std::max(perception_radius - 5.f, 0.f);
                        break;
                    case Control::ANGLE:
                        perception_angle = std::max(perception_angle - 5_deg, 0_deg);
                        break;
                    }
                    break;
                default:
                    break;
                }
                break;
            case sf::Event::MouseButtonPressed:
                if (event.mouseButton.button == sf::Mouse::Left) {
                    const auto mouse = sf::Vector2f(float(event.mouseButton.x), float(event.mouseButton.y));
                    auto min_distance = std::numeric_limits<float>::max();
                    for (auto& boid : boids) {
                        const auto distance = (boid.getPosition() - mouse).lengthSq();
                        if (distance < min_distance) {
                            min_distance = distance;
                            selected_boid = &boid;
                        }
                    }
                    selected_boid->select();
                }
                break;
            default:
                break;
            }
        }

        window.clear();

        auto highlighted_neighbors = std::vector<Boid*>();
        for (auto& boid : boids) {
            auto neighbors = std::vector<Boid*>();
            for (auto& neighbor : boids)
                if (boid.can_see(neighbor, perception_radius, perception_angle))
                    neighbors.push_back(&neighbor);
            boid.flock(neighbors, gain, video_mode.size);
            if (&boid == selected_boid)
                highlighted_neighbors = neighbors;
            else
                boid.reset_color();
        }

        for (auto& neighbor : highlighted_neighbors)
            neighbor->highlight();

        window.setView(content_view);
        const auto elapsed = clock.restart();
        for (auto& boid : boids) {
            boid.update(elapsed);
            window.draw(boid);
        }
        window.draw(*selected_boid);

        auto view_region = sf::ConvexShape(100);
        view_region.setFillColor({ 255, 255, 255, 64 });
        const auto delta_theta = 2 * perception_angle / float(view_region.getPointCount() - 2);
        const auto initial_angle = 90_deg - perception_angle - delta_theta;
        for (size_t i = 1; i < view_region.getPointCount(); ++i) {
            const auto theta = initial_angle + float(i) * delta_theta;
            const auto point = perception_radius * -sf::Vector2f::UnitY.rotatedBy(theta);
            view_region.setPoint(i, point);
        }
        view_region.setPosition(selected_boid->getPosition());
        view_region.setRotation(selected_boid->getRotation());
        window.draw(view_region);

        auto text_builder = std::ostringstream();
        text_builder << std::setprecision(1) << std::scientific;
        text_builder << gain.alignment << " (A) alignment" << (control == Control::ALIGNMENT ? " <" : "") << '\n';
        text_builder << gain.cohesion << " (C) cohesion" << (control == Control::COHESION ? " <" : "") << '\n';
        text_builder << gain.separation << " (S) separation" << (control == Control::SEPARATION ? " <" : "") << '\n';
        text_builder << std::setprecision(0) << std::fixed;
        text_builder << perception_radius << " (R) radius" << (control == Control::RADIUS ? " <" : "") << '\n';
        text_builder << 2 * perception_angle.asDegrees() << " (G) angle" << (control == Control::ANGLE ? " <" : "")
                     << '\n';
        text_builder << std::setw(3) << 1.f / elapsed.asSeconds() << " fps\n";
        text.setString(text_builder.str());

        window.setView(overlay_view);
        window.draw(text);

        window.display();
    }
}
