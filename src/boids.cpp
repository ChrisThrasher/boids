#include <SFML/Graphics.hpp>

#include <array>
#include <cmath>
#include <iomanip>
#include <numeric>
#include <random>
#include <sstream>

static constexpr auto width = 1920u;
static constexpr auto height = 1080u;
static constexpr auto framerate = 60;
static constexpr auto min_speed = 250.0f;
static constexpr auto max_speed = 500.0f;
static constexpr auto pi = 3.1415926f;
static constexpr auto to_radians = pi / 180.0f;
static constexpr auto to_degrees = 180.0f / pi;

static auto random_device = std::random_device();
static auto rng = std::mt19937(random_device());
static auto x_position_dist = std::uniform_real_distribution<float>(0.0f, width);
static auto y_position_dist = std::uniform_real_distribution<float>(0.0f, height);
static auto rotation_dist = std::uniform_real_distribution<float>(0.0f, 360.0f);
static auto velocity_dist = std::uniform_real_distribution<float>(min_speed, max_speed);
static auto brightness_dist = std::uniform_int_distribution<sf::Uint8>(128, 255);

static auto alignment_gain = 4e1f;
static auto cohesion_gain = 4e2f;
static auto separation_gain = 2e6f;
static auto perception_radius = 100.0f;
static auto perception_angle = 135.0f * to_radians;

static auto Dot(const sf::Vector2f& lhs, const sf::Vector2f& rhs) { return lhs.x * rhs.x + lhs.y * rhs.y; }

static auto Length2(const sf::Vector2f& vector) noexcept { return Dot(vector, vector); }

static auto Length(const sf::Vector2f& vector) noexcept { return std::sqrt(Length2(vector)); }

static auto Clamp(const sf::Vector2f& vector, const float min, const float max)
{
    const auto length = Length(vector);
    if (length == 0.0f)
        return vector;
    return vector * std::clamp(length, min, max) / length;
}

class Boid : public sf::ConvexShape {
    sf::Vector2f m_velocity {};
    sf::Vector2f m_acceleration {};
    sf::Color m_color {};

public:
    Boid();
    auto GetVelocity() const -> const auto& { return m_velocity; }

    void Flock(const std::vector<Boid*>& neighbors);
    void Update(const float dt);
    void Select() { setFillColor(sf::Color::Red); }
    void Highlight() { setFillColor(sf::Color::Yellow); }
    void ResetColor() { setFillColor(m_color); }
};

Boid::Boid()
    : sf::ConvexShape(4)
{
    setPoint(0, { 2, 0 });
    setPoint(1, { -2, -2 });
    setPoint(2, { -1, 0 });
    setPoint(3, { -2, 2 });
    setScale(10.0f, 10.0f);
    setPosition({ x_position_dist(rng), y_position_dist(rng) });
    setRotation(rotation_dist(rng));
    const auto brightness = brightness_dist(rng);
    m_color = { brightness, brightness, brightness };
    ResetColor();

    m_velocity = velocity_dist(rng)
        * sf::Vector2f { std::cos(getRotation() * to_radians), std::sin(getRotation() * to_radians) };
}

void Boid::Flock(const std::vector<Boid*>& neighbors)
{
    m_acceleration = {};
    if (neighbors.empty())
        return;

    const auto for_all_neighbors = [neighbors](const auto& transform) {
        return std::accumulate(neighbors.begin(), neighbors.end(), sf::Vector2f {}, transform);
    };

    const auto alignment = for_all_neighbors([this](const sf::Vector2f& sum, const Boid* boid) {
                               return sum + boid->GetVelocity() - GetVelocity();
                           })
        * alignment_gain;
    const auto cohesion = for_all_neighbors([this](const sf::Vector2f& sum, const Boid* boid) {
                              return sum + boid->getPosition() - getPosition();
                          })
        * cohesion_gain;
    const auto separation = for_all_neighbors([this](const sf::Vector2f& sum, const Boid* boid) {
                                const auto diff = getPosition() - boid->getPosition();
                                return sum + diff / Length2(diff);
                            })
        * separation_gain;

    m_acceleration = (alignment + cohesion + separation) / (float)neighbors.size();
    m_acceleration = Clamp(m_acceleration, 0.0f, 500.0f);
}

void Boid::Update(const float dt)
{
    move(dt * m_velocity);
    m_velocity = Clamp(m_velocity + dt * m_acceleration, min_speed, max_speed);

    setRotation(std::atan2(m_velocity.y, m_velocity.x) * to_degrees);
    setPosition(fmodf(getPosition().x + width, width), fmodf(getPosition().y + height, height));
}

int main(int argc, char* argv[])
{
    size_t num_boids = 250;
    if (argc > 1)
        num_boids = std::stoull(argv[1]);
    auto boids = std::vector<Boid>(num_boids);
    auto* selected_boid = &boids.front();
    selected_boid->Select();

    auto clock = sf::Clock();
    auto font = sf::Font();
    font.loadFromFile(std::string(FONT_PATH) + "/font.ttf");

    auto text = sf::Text("", font, 24);
    text.setFillColor(sf::Color::White);
    text.setOutlineThickness(2);
    text.setOutlineColor(sf::Color::Black);
    text.setPosition({ 10.0f, 5.0f });

    auto window = sf::RenderWindow(sf::VideoMode(width, height), std::to_string(num_boids) + " Boids");
    window.setFramerateLimit(framerate);

    enum class Control { ALIGNMENT, COHESION, SEPARATION, RADIUS, ANGLE } control = Control::ALIGNMENT;

    while (window.isOpen()) {
        auto event = sf::Event();
        while (window.pollEvent(event)) {
            switch (event.type) {
            case sf::Event::Closed:
                window.close();
                break;
            case sf::Event::KeyPressed:
                switch (event.key.code) {
                case sf::Keyboard::Space:
                    boids = std::vector<Boid>(num_boids);
                    selected_boid = &boids.front();
                    selected_boid->Select();
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
                        alignment_gain *= 2.0f;
                        break;
                    case Control::COHESION:
                        cohesion_gain *= 2.0f;
                        break;
                    case Control::SEPARATION:
                        separation_gain *= 2.0f;
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
                        alignment_gain = std::max(alignment_gain / 2.0f, 1.0f);
                        break;
                    case Control::COHESION:
                        cohesion_gain = std::max(cohesion_gain / 2.0f, 1.0f);
                        break;
                    case Control::SEPARATION:
                        separation_gain = std::max(separation_gain / 2.0f, 1.0f);
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
                        const auto distance = Length2(boid.getPosition() - mouse);
                        if (distance < min_distance) {
                            min_distance = distance;
                            selected_boid = &boid;
                        }
                    }
                    selected_boid->Select();
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
            for (auto& neighbor : boids) {
                if (&boid == &neighbor)
                    continue;
                const auto to_neighbor = neighbor.getPosition() - boid.getPosition();
                const auto is_within_radius = Length2(to_neighbor) < perception_radius * perception_radius;
                if (is_within_radius) {
                    const auto is_within_view
                        = Dot(to_neighbor, boid.GetVelocity()) / (Length(to_neighbor) * Length(boid.GetVelocity()))
                        > std::cos(perception_angle);
                    if (is_within_view)
                        neighbors.push_back(&neighbor);
                }
            }
            boid.Flock(neighbors);
            if (&boid == selected_boid)
                highlighted_neighbors = neighbors;
            else
                boid.ResetColor();
        }

        for (auto& neighbor : highlighted_neighbors)
            neighbor->Highlight();

        const auto elapsed = clock.restart().asSeconds();
        for (auto& boid : boids) {
            boid.Update(elapsed);
            window.draw(boid);
        }
        window.draw(*selected_boid);

        auto view_region = sf::ConvexShape(30);
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
        text_builder << alignment_gain << " (A) alignment" << (control == Control::ALIGNMENT ? " <" : "") << '\n';
        text_builder << cohesion_gain << " (C) cohesion" << (control == Control::COHESION ? " <" : "") << '\n';
        text_builder << separation_gain << " (S) separation" << (control == Control::SEPARATION ? " <" : "") << '\n';
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
