#include <SFML/Graphics.hpp>

#include <array>
#include <cmath>
#include <iomanip>
#include <numbers>
#include <numeric>
#include <random>
#include <sstream>

static constexpr auto width = 1920u;
static constexpr auto height = 1080u;
static constexpr auto framerate = 60;
static constexpr auto min_speed = 250.0f;
static constexpr auto max_speed = 500.0f;
static constexpr auto to_radians = std::numbers::pi_v<float> / 180.0f;
static constexpr auto to_degrees = 180.0f / std::numbers::pi_v<float>;

static auto random_device = std::random_device();
static auto rng = std::mt19937(random_device());
static auto x_position_dist = std::uniform_real_distribution<float>(0.0f, width);
static auto y_position_dist = std::uniform_real_distribution<float>(0.0f, height);
static auto rotation_dist = std::uniform_real_distribution<float>(0.0f, 360.0f);
static auto velocity_dist = std::uniform_real_distribution<float>(min_speed, max_speed);
static auto brightness_dist = std::uniform_int_distribution<uint8_t>(128, 255);

static auto alignment_gain = 4e1f;
static auto cohesion_gain = 4e2f;
static auto separation_gain = 2e6f;
static auto perception_radius = 100.0f;
static auto perception_angle = 135.0f * to_radians;

static auto Dot(const sf::Vector2f& lhs, const sf::Vector2f& rhs) { return lhs.x * rhs.x + lhs.y * rhs.y; }

static auto Length(const sf::Vector2f& vector) noexcept { return std::sqrt(vector.x * vector.x + vector.y * vector.y); }

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
    bool m_is_selected { false };
    bool m_is_highlighted { false };
    std::vector<Boid*> m_neighbors;

    void Align();
    void Cohere();
    void Separate();

public:
    Boid();
    auto GetVelocity() const { return m_velocity; }
    auto IsSelected() const { return m_is_selected; }

    void SetNeighbors(const std::vector<Boid*>& neighbors);
    void Update(const float dt);
    void Select();
    void Deselect();
    void Highlight();
    void Dehighlight();
};

Boid::Boid()
{
    setScale(10.0f, 10.0f);
    setPosition({ x_position_dist(rng), y_position_dist(rng) });
    setRotation(rotation_dist(rng));
    setPointCount(4);
    setPoint(0, { 2, 0 });
    setPoint(1, { -2, -2 });
    setPoint(2, { -1, 0 });
    setPoint(3, { -2, 2 });
    const auto brightness = brightness_dist(rng);
    setFillColor({ brightness, brightness, brightness });

    m_velocity = velocity_dist(rng)
        * sf::Vector2f { std::cos(getRotation() * to_radians), std::sin(getRotation() * to_radians) };
}

void Boid::Align()
{
    const auto alignment = std::accumulate(m_neighbors.begin(),
                                           m_neighbors.end(),
                                           sf::Vector2f(),
                                           [this](const sf::Vector2f& sum, const Boid* boid) {
                                               return sum + boid->GetVelocity() - GetVelocity();
                                           })
        / (float)m_neighbors.size();
    m_acceleration += alignment_gain * alignment;
}

void Boid::Cohere()
{
    const auto cohesion = std::accumulate(m_neighbors.begin(),
                                          m_neighbors.end(),
                                          sf::Vector2f(),
                                          [this](const sf::Vector2f& sum, const Boid* boid) {
                                              return sum + boid->getPosition() - getPosition();
                                          })
        / (float)m_neighbors.size();
    m_acceleration += cohesion_gain * cohesion;
}

void Boid::Separate()
{
    const auto separation = std::accumulate(m_neighbors.begin(),
                                            m_neighbors.end(),
                                            sf::Vector2f(),
                                            [this](const sf::Vector2f& sum, const Boid* boid) {
                                                const auto diff = getPosition() - boid->getPosition();
                                                const auto length2 = Length(diff) * Length(diff);
                                                return sum + diff / length2;
                                            })
        / (float)m_neighbors.size();
    m_acceleration += separation_gain * separation;
}

void Boid::Update(const float dt)
{
    move(dt * m_velocity);
    m_acceleration = Clamp(m_acceleration, 0.0f, 500.0f);
    m_velocity = Clamp(m_velocity + dt * m_acceleration, min_speed, max_speed);
    setRotation(std::atan2(m_velocity.y, m_velocity.x) * to_degrees);
    m_acceleration = {};

    if (getPosition().x > width)
        setPosition(0, getPosition().y);
    if (getPosition().x < 0)
        setPosition(width, getPosition().y);
    if (getPosition().y > height)
        setPosition(getPosition().x, 0);
    if (getPosition().y < 0)
        setPosition(getPosition().x, height);
}

void Boid::Select()
{
    Dehighlight();
    m_is_selected = true;
    setFillColor(sf::Color::Red);
    for (auto* neighbor : m_neighbors)
        neighbor->Highlight();
}

void Boid::Deselect()
{
    if (!m_is_selected)
        return;
    m_is_selected = false;
    const auto brightness = brightness_dist(rng);
    setFillColor({ brightness, brightness, brightness });
    for (auto* neighbor : m_neighbors)
        neighbor->Dehighlight();
}

void Boid::Highlight()
{
    Deselect();
    m_is_highlighted = true;
    setFillColor(sf::Color::Yellow);
}

void Boid::Dehighlight()
{
    if (!m_is_highlighted)
        return;
    m_is_highlighted = false;
    const auto brightness = brightness_dist(rng);
    setFillColor({ brightness, brightness, brightness });
}

void Boid::SetNeighbors(const std::vector<Boid*>& neighbors)
{
    m_neighbors = neighbors;
    if (m_neighbors.empty())
        return;
    Align();
    Cohere();
    Separate();
}

int main()
{
    auto boids = std::array<Boid, 250>();
    boids.front().Select();

    auto clock = sf::Clock();
    auto font = sf::Font();
    font.loadFromFile(std::string(FONT_PATH) + "/font.ttf");

    auto text = sf::Text("", font, 24);
    text.setFillColor(sf::Color::White);
    text.setOutlineThickness(2);
    text.setOutlineColor(sf::Color::Black);
    text.setPosition({ 10.0f, 5.0f });

    auto window = sf::RenderWindow(sf::VideoMode(width, height), "Boids");
    window.setFramerateLimit(framerate);

    enum class Control { ALIGNMENT, COHESION, SEPARATION, RADIUS, ANGLE } active_control = Control::ALIGNMENT;

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
                    boids = {};
                    boids.front().Select();
                    break;
                case sf::Keyboard::A:
                    active_control = Control::ALIGNMENT;
                    break;
                case sf::Keyboard::C:
                    active_control = Control::COHESION;
                    break;
                case sf::Keyboard::S:
                    active_control = Control::SEPARATION;
                    break;
                case sf::Keyboard::R:
                    active_control = Control::RADIUS;
                    break;
                case sf::Keyboard::G:
                    active_control = Control::ANGLE;
                    break;
                case sf::Keyboard::Up:
                    switch (active_control) {
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
                        perception_angle += 5.0f * to_radians;
                        break;
                    }
                    break;
                case sf::Keyboard::Down:
                    switch (active_control) {
                    case Control::ALIGNMENT:
                        alignment_gain /= 2.0f;
                        break;
                    case Control::COHESION:
                        cohesion_gain /= 2.0f;
                        break;
                    case Control::SEPARATION:
                        separation_gain /= 2.0f;
                        break;
                    case Control::RADIUS:
                        perception_radius -= 5.0f;
                        break;
                    case Control::ANGLE:
                        perception_angle -= 5.0f * to_radians;
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
                    auto* closest_boid = &boids.front();
                    for (auto& boid : boids) {
                        boid.Deselect();
                        const auto distance = Length(boid.getPosition() - mouse);
                        if (distance < min_distance) {
                            min_distance = distance;
                            closest_boid = &boid;
                        }
                    }
                    closest_boid->Select();
                }
                break;
            default:
                break;
            }
        }

        window.clear();

        for (auto& boid : boids) {
            auto neighbors = std::vector<Boid*>();
            for (auto& neighbor : boids) {
                if (&boid == &neighbor)
                    continue;
                const auto is_within_radius = Length(neighbor.getPosition() - boid.getPosition()) < perception_radius;
                const auto to_neighbor = neighbor.getPosition() - boid.getPosition();
                const auto is_within_view = std::acos(Dot(to_neighbor, boid.GetVelocity())
                                                      / (Length(to_neighbor) * Length(boid.GetVelocity())))
                    < perception_angle;
                if (is_within_radius && is_within_view) {
                    neighbors.push_back(&neighbor);
                    if (boid.IsSelected())
                        neighbor.Highlight();
                } else {
                    if (boid.IsSelected())
                        neighbor.Dehighlight();
                }
            }
            boid.SetNeighbors(neighbors);
        }

        const auto elapsed = clock.getElapsedTime();
        clock.restart();
        for (auto& boid : boids) {
            boid.Update(elapsed.asSeconds());
            window.draw(boid);
            if (boid.IsSelected()) {
                auto view_region = sf::ConvexShape();
                view_region.setPointCount(30);
                view_region.setFillColor({ 255, 255, 255, 64 });
                const auto initial_angle = std::numbers::pi_v<float> / 2.0f - perception_angle;
                const auto delta_theta = 2 * perception_angle / (float)view_region.getPointCount();
                for (size_t i = 1; i < view_region.getPointCount(); ++i) {
                    const auto theta = initial_angle + (float)i * delta_theta;
                    const auto point = perception_radius * sf::Vector2f(std::sin(theta), std::cos(theta));
                    view_region.setPoint(i, point);
                }
                view_region.setPosition(boid.getPosition());
                view_region.setRotation(boid.getRotation());
                window.draw(view_region);
            }
        }

        auto text_builder = std::ostringstream();
        text_builder << std::setprecision(1) << std::scientific;
        text_builder << alignment_gain << " (A) alignment" << (active_control == Control::ALIGNMENT ? " <" : "")
                     << '\n';
        text_builder << cohesion_gain << " (C) cohesion" << (active_control == Control::COHESION ? " <" : "") << '\n';
        text_builder << separation_gain << " (S) separation" << (active_control == Control::SEPARATION ? " <" : "")
                     << '\n';
        text_builder << std::fixed;
        text_builder << perception_radius << " (R) radius" << (active_control == Control::RADIUS ? " <" : "") << '\n';
        text_builder << perception_angle * to_degrees << " (G) angle" << (active_control == Control::ANGLE ? " <" : "")
                     << '\n';
        text_builder << std::setw(3) << 1'000'000 / elapsed.asMicroseconds() << " fps\n";
        text.setString(text_builder.str());

        window.draw(text);
        window.display();
    }
}
