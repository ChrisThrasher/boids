#include "boid.h"
#include "vector_utils.h"

#include <array>
#include <iomanip>
#include <numeric>
#include <random>
#include <sstream>
#include <vector>

static const auto video_mode = sf::VideoMode(1920, 1080);

static auto make_boids(const size_t num_boids)
{
    static auto rng = []() {
        auto seed_data = std::array<int, std::mt19937::state_size>();
        auto rd = std::random_device();
        std::generate_n(seed_data.data(), seed_data.size(), std::ref(rd));
        auto seed_seq = std::seed_seq(seed_data.begin(), seed_data.end());
        return std::mt19937(seed_seq);
    }();
    static auto x_position_dist = std::uniform_real_distribution<float>(0.0f, (float)video_mode.width);
    static auto y_position_dist = std::uniform_real_distribution<float>(0.0f, (float)video_mode.height);
    static auto rotation_dist = std::uniform_real_distribution<float>(0.0f, 360.0f);

    auto boids = std::vector<Boid>();
    for (size_t i = 0; i < num_boids; ++i)
        boids.emplace_back(sf::Vector2f(x_position_dist(rng), y_position_dist(rng)), rotation_dist(rng));
    return boids;
}

int main(int argc, char* argv[])
{
    size_t num_boids = 250;
    if (argc > 1)
        num_boids = std::stoull(argv[1]);
    auto boids = make_boids(num_boids);
    auto* selected_boid = &boids.front();
    selected_boid->select();

    auto gain = Boid::Gain { 4e1f, 4e2f, 2e6f };
    auto perception_radius = 100.0f;
    auto perception_angle = 135.0f * to_radians;

    auto clock = sf::Clock();
    auto font = sf::Font();
    font.loadFromFile(std::string(FONT_PATH) + "/font.ttf");

    auto text = sf::Text("", font, 24);
    text.setFillColor(sf::Color::White);
    text.setOutlineThickness(2);
    text.setOutlineColor(sf::Color::Black);
    text.setPosition({ 10.0f, 5.0f });

    auto window = sf::RenderWindow(video_mode, std::to_string(num_boids) + " Boids");
    window.setFramerateLimit(60);

    enum class Control { ALIGNMENT, COHESION, SEPARATION, RADIUS, ANGLE } control = Control::ALIGNMENT;

    while (window.isOpen()) {
        for (auto event = sf::Event(); window.pollEvent(event);) {
            switch (event.type) {
            case sf::Event::Closed:
                window.close();
                break;
            case sf::Event::KeyPressed:
                switch (event.key.code) {
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
                        gain.alignment = std::min(gain.alignment * 2.0f, 1e16f);
                        break;
                    case Control::COHESION:
                        gain.cohesion = std::min(gain.cohesion * 2.0f, 1e16f);
                        break;
                    case Control::SEPARATION:
                        gain.separation = std::min(gain.separation * 2.0f, 1e16f);
                        break;
                    case Control::RADIUS:
                        perception_radius += 5.0f;
                        break;
                    case Control::ANGLE:
                        perception_angle = std::min(perception_angle + 5.0f * to_radians, 180.0f * to_radians);
                        break;
                    }
                    break;
                case sf::Keyboard::Down:
                    switch (control) {
                    case Control::ALIGNMENT:
                        gain.alignment = std::max(gain.alignment / 2.0f, 1.0f);
                        break;
                    case Control::COHESION:
                        gain.cohesion = std::max(gain.cohesion / 2.0f, 1.0f);
                        break;
                    case Control::SEPARATION:
                        gain.separation = std::max(gain.separation / 2.0f, 1.0f);
                        break;
                    case Control::RADIUS:
                        perception_radius = std::max(perception_radius - 5.0f, 0.0f);
                        break;
                    case Control::ANGLE:
                        perception_angle = std::max(perception_angle - 5.0f * to_radians, 0.0f);
                        break;
                    }
                    break;
                default:
                    break;
                }
                break;
            case sf::Event::MouseButtonPressed:
                if (event.mouseButton.button == sf::Mouse::Left) {
                    const auto mouse = sf::Vector2f { (float)event.mouseButton.x, (float)event.mouseButton.y };
                    auto min_distance = std::numeric_limits<float>::max();
                    for (auto& boid : boids) {
                        const auto distance = length2(boid.getPosition() - mouse);
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
            boid.flock(neighbors, gain);
            if (&boid == selected_boid)
                highlighted_neighbors = neighbors;
            else
                boid.reset_color();
        }

        for (auto& neighbor : highlighted_neighbors)
            neighbor->highlight();

        const auto elapsed = clock.restart().asSeconds();
        for (auto& boid : boids) {
            boid.update(elapsed, video_mode);
            window.draw(boid);
        }
        window.draw(*selected_boid);

        auto view_region = sf::ConvexShape(100);
        view_region.setFillColor({ 255, 255, 255, 64 });
        const auto delta_theta = 2 * perception_angle / (float)(view_region.getPointCount() - 2);
        const auto initial_angle = pi / 2.0f - perception_angle - delta_theta;
        for (size_t i = 1; i < view_region.getPointCount(); ++i) {
            const auto theta = initial_angle + (float)i * delta_theta;
            const auto point = perception_radius * sf::Vector2f(std::sin(theta), std::cos(theta));
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
        text_builder << 2 * perception_angle * to_degrees << " (G) angle" << (control == Control::ANGLE ? " <" : "")
                     << '\n';
        text_builder << std::setw(3) << 1.0f / elapsed << " fps\n";
        text.setString(text_builder.str());
        window.draw(text);

        window.display();
    }
}
