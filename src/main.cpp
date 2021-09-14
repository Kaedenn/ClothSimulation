#include "config.hpp"

/* TODO: Command-line handling
 *  wind definition(s):
 *    tuple<3, V2f> wind_defn (multi)
 *  initial focus (RenderContext::setFocus(sf::Vector2f focus))
 *  initial zoom (RenderContext::setZoom(float zoom))
 *  have command-line values override configuration file
 *    requires removing default values (or prepopulating them with config file values)
 * TODO: fix initial focus center to work regardless of cloth height
 * TODO: swap move and cut (cut=lmb, move=mmb)?
 *  cut: lmb?
 *  move: shift+lmb ?
 * TODO: keybind to reset simulation?
 * TODO: configure cut radius (default 10)
 * TODO: color feedback
 * TODO: cloth variants
 */

bool isInRadius(const Particle& p, sf::Vector2f center, float radius);

void applyForceOnCloth(sf::Vector2f position, float radius, sf::Vector2f force, PhysicSolver& solver);

int main(int argc, char* argv[])
{
    config conf = config();
    switch (conf.parseCommandLineArguments(argc, argv)) {
        case config::Status::OK:
            break;
        case config::Status::EXIT:
            return 0;
        case config::Status::ERROR:
            return 1;
    }

    if (conf.debug) {
        conf.print(std::cerr);
    }

    const sf::Vector2u window_size(conf.window_width, conf.window_height);
    WindowContextHandler app("Cloth", window_size, sf::Style::Default);

    PhysicSolver solver(conf.gravity_x, conf.gravity_y, conf.friction_coef);
    Renderer renderer(solver);

    conf.buildCloth(solver);

    app.getRenderContext().setZoom(conf.initial_zoom);

    sf::Vector2f last_mouse_position;
    bool dragging = false;
    bool erasing = false;
    bool wind_blowing = true;
    // Add events callback for mouse control
    app.getEventManager().addMousePressedCallback(sf::Mouse::Right, [&](sfev::CstEv) {
        dragging = true;
        last_mouse_position = app.getWorldMousePosition();
    });
    app.getEventManager().addMouseReleasedCallback(sf::Mouse::Right, [&](sfev::CstEv) {
        dragging = false;
    });
    app.getEventManager().addMousePressedCallback(sf::Mouse::Middle, [&](sfev::CstEv) {
        erasing = true;
    });
    app.getEventManager().addMouseReleasedCallback(sf::Mouse::Middle, [&](sfev::CstEv) {
        erasing = false;
    });
    // Add events callback for additional keyboard controls
    app.getEventManager().addKeyPressedCallback(sf::Keyboard::Key::Space, [&](sfev::CstEv) {
        wind_blowing = !wind_blowing;
        std::cerr << "Wind is " << (wind_blowing ? "now" : "no longer") << " blowing" << std::endl;
    });
    app.getEventManager().addKeyPressedCallback(sf::Keyboard::Key::Slash, [&](sfev::CstEv) {
        ViewportHandler::State vstate = app.getRenderContext().getState();
        std::cerr << "current viewport state:"
            << "\ncenter: " << vstate.center.x << ", " << vstate.center.y
            << "\nzoom: " << vstate.zoom
            << "\noffset: " << vstate.offset.x << "," << vstate.offset.y
            << std::endl;
    });
    /* TODO: rebuild cloth
    app.getEventManager().addKeyPressedCallback(sf::Keyboard::Key::Enter, [&](sfev::CstEv) {
        buildCloth(solver, conf);
        std::cerr << "Rebuilt cloth" << std::endl;
    });
    */

    WindManager wind(to<float>(conf.window_width));
    if (conf.winds.size() == 0) {
        if (!conf.disable_default_wind) {
            // Add 2 wind waves
            wind.winds.emplace_back(
                sf::Vector2f(100.0f, conf.window_height),
                sf::Vector2f(0.0f, 0.0f),
                sf::Vector2f(1000.0f, 0.0f)
            );
            wind.winds.emplace_back(
                sf::Vector2f(20.0f, conf.window_height),
                sf::Vector2f(0.0f, 0.0f),
                sf::Vector2f(3000.0f, 0.0f)
            );
        }
    } else {
        for (const Wind& w : conf.winds) {
            wind.winds.push_back(w);
        }
    }

    // Main loop
    const float dt = 1.0f / 60.0f;
    while (app.run()) {
        // Get the mouse coord in the world space, to allow proper control even with modified viewport
        const sf::Vector2f mouse_position = app.getWorldMousePosition();

        if (dragging) {
            // Apply a force on the particles in the direction of the mouse's movement
            const sf::Vector2f mouse_speed = mouse_position - last_mouse_position;
            const sf::Vector2f mouse_force = mouse_speed * conf.mouse_drag_force;
            last_mouse_position = mouse_position;
            applyForceOnCloth(mouse_position, conf.mouse_drag_radius, mouse_force, solver);
        }

        if (erasing) {
            // Delete all nodes that are in the range of the mouse
            for (Particle& p : solver.objects) {
                if (isInRadius(p, mouse_position, conf.erase_radius)) {
                    solver.objects.erase(p.id);
                }
            }
        }
        // Update physics
        if (wind_blowing) {
            wind.update(solver, dt);
        }
        solver.update(dt);
        // Render the scene
        RenderContext& render_context = app.getRenderContext();
        render_context.clear();
        renderer.render(render_context);
        render_context.display();
    }

    return 0;
}

bool isInRadius(const Particle& p, sf::Vector2f center, float radius)
{
    const sf::Vector2f v = center - p.position;
    return v.x * v.x + v.y * v.y < radius * radius;
}

void applyForceOnCloth(sf::Vector2f position, float radius, sf::Vector2f force, PhysicSolver& solver)
{
    for (Particle& p : solver.objects) {
        if (isInRadius(p, position, radius)) {
            p.forces += force;
        }
    }
}

/* vim: set ts=4 sts=4 sw=4 et: */
