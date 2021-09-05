#include <sstream>
#include <iostream>
#include <boost/program_options.hpp>
#include "engine/window_context_handler.hpp"
#include "engine/physics/physics.hpp"
#include "renderer.hpp"
#include "wind.hpp"

namespace po = boost::program_options;

const uint32_t WINDOW_WIDTH_DEFAULT = 1920;
const uint32_t WINDOW_HEIGHT_DEFAULT = 1080;
const uint32_t CLOTH_WIDTH_DEFAULT = 75;
const uint32_t CLOTH_HEIGHT_DEFAULT = 50;
const float LINKS_LENGTH_DEFAULT = 20.0f;

/* TODO: Command-line handling
 *  V2f PhysicsSolver gravity (default (0.0f, 1500.0f))
 *  float PhysicsSolver friction (default 0.5f)
 *  wind definition(s):
 *    default wind (default)
 *    no wind
 *    tuple<3, V2f> wind_defn (multi)
 * TODO: swap move and cut (cut=lmb, move=mmb)?
 * TODO: keybind to reset simulation?
 * TODO: configure cut radius (default 10)
 * TODO: color feedback
 *  add color to Particle struct
 *  set color in renderer via the Vertex instances
 *    https://www.sfml-dev.org/documentation/2.5.1/classsf_1_1Vertex.php
 */

struct config {
    config()
        : want_exit(false)
        , has_error(false)
        , debug(false)
        , window_width(WINDOW_WIDTH_DEFAULT)
        , window_height(WINDOW_HEIGHT_DEFAULT)
        , cloth_width(CLOTH_WIDTH_DEFAULT)
        , cloth_height(CLOTH_HEIGHT_DEFAULT)
        , links_length(LINKS_LENGTH_DEFAULT)
        , gravity_x(GRAVITY_X_DEFAULT)
        , gravity_y(GRAVITY_Y_DEFAULT)
        , friction_coef(FRICTION_DEFAULT)
        , disable_default_wind(false)
    {}
    bool want_exit;
    bool has_error;
    bool debug;
    uint32_t window_width;
    uint32_t window_height;
    uint32_t cloth_width;
    uint32_t cloth_height;
    float links_length;
    float gravity_x;
    float gravity_y;
    float friction_coef;
    bool disable_default_wind;
};

config parseCommandLineArguments(int argc, char* argv[]);

bool isInRadius(const Particle& p, sf::Vector2f center, float radius);

void applyForceOnCloth(sf::Vector2f position, float radius, sf::Vector2f force, PhysicSolver& solver);

int main(int argc, char* argv[])
{
    config conf = parseCommandLineArguments(argc, argv);
    if (conf.want_exit) {
        return 0;
    } else if (conf.has_error) {
        return 1;
    }

    if (conf.debug) {
        std::cerr << "parameters:" << std::endl
                  << "window size: " << conf.window_width << "," << conf.window_height << std::endl
                  << "cloth size: " << conf.cloth_width << " by " << conf.cloth_height << std::endl
                  << "link length: " << conf.links_length << std::endl
                  << "gravity vector: " << conf.gravity_x << ", " << conf.gravity_y << std::endl
                  << "friction coefficient: " << conf.friction_coef << std::endl;
    }

    const sf::Vector2u window_size(conf.window_width, conf.window_height);
    WindowContextHandler app("Cloth", window_size, sf::Style::Default);

    PhysicSolver solver(conf.gravity_x, conf.gravity_y, conf.friction_coef);
    Renderer renderer(solver);

    const float start_x = (conf.window_width - (conf.cloth_width - 1) * conf.links_length) * 0.5f;
    // Initialize the cloth
    for (uint32_t y(0); y < conf.cloth_height; ++y) {
        const float max_elongation = 1.2f * (2.0f - y / float(conf.cloth_height));
        for (uint32_t x(0); x < conf.cloth_width; ++x) {
            const civ::ID id = solver.addParticle(
                sf::Vector2f(start_x + x * conf.links_length, y * conf.links_length)
            );
            // Add left link if there is a particle on the left
            if (x > 0) {
                solver.addLink(id-1, id, max_elongation * 0.9f);
            }
            // Add top link if there is a particle on the top
            if (y > 0) {
                solver.addLink(id-conf.cloth_width, id, max_elongation);
            } else {
                // If not, pin the particle
                solver.objects[id].moving = false;
            }
        }
    }

    sf::Vector2f last_mouse_position;
    bool dragging = false;
    bool erasing = false;
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

    WindManager wind(to<float>(conf.window_width));
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

    // Main loop
    const float dt = 1.0f / 60.0f;
    while (app.run()) {
        // Get the mouse coord in the world space, to allow proper control even with modified viewport
        const sf::Vector2f mouse_position = app.getWorldMousePosition();

        if (dragging) {
            // Apply a force on the particles in the direction of the mouse's movement
            const sf::Vector2f mouse_speed = mouse_position - last_mouse_position;
            last_mouse_position = mouse_position;
            applyForceOnCloth(mouse_position, 100.0f, mouse_speed * 8000.0f, solver);
        }

        if (erasing) {
            // Delete all nodes that are in the range of the mouse
            for (Particle& p : solver.objects) {
                if (isInRadius(p, mouse_position, 10.0f)) {
                    solver.objects.erase(p.id);
                }
            }
        }
        // Update physics
        wind.update(solver, dt);
        solver.update(dt);
        // Render the scene
        RenderContext& render_context = app.getRenderContext();
        render_context.clear();
        renderer.render(render_context);
        render_context.display();
    }

    return 0;
}

config parseCommandLineArguments(int argc, char* argv[])
{
    po::options_description opts("general options");
    opts.add_options()
        ("verbose,v", "enable debugging output")
        ("help,h", "produce help message")
        ("wsize", po::value<uint32_t>()->default_value(WINDOW_WIDTH_DEFAULT),
         "window width in pixels")
        ("hsize", po::value<uint32_t>()->default_value(WINDOW_HEIGHT_DEFAULT),
         "window height in pixels");
    po::options_description phys_opts("physics options");
    phys_opts.add_options()
        ("width,W", po::value<uint32_t>()->default_value(CLOTH_WIDTH_DEFAULT),
         "cloth mesh horizontal size")
        ("height,H", po::value<uint32_t>()->default_value(CLOTH_HEIGHT_DEFAULT),
         "cloth mesh vertical size")
        ("linksize,l", po::value<float>()->default_value(LINKS_LENGTH_DEFAULT),
         "cloth links length")
        ("gx", po::value<float>()->default_value(GRAVITY_X_DEFAULT),
         "gravity horizontal component")
        ("gy", po::value<float>()->default_value(GRAVITY_Y_DEFAULT),
         "gravity vertical component (positive = down)")
        ("friction,f", po::value<float>()->default_value(FRICTION_DEFAULT),
         "friction coefficient")
        ("nowind,N", "disable wind")
        ;
    opts.add(phys_opts);
    config conf;
    try {
        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, opts), vm);
        vm.notify();
        if (vm.count("help")) {
            std::cerr << "usage: " << argv[0] << std::endl;
            opts.print(std::cerr);
            conf.want_exit = true;
        }
        conf.debug = vm.count("verbose") > 0;
        conf.window_width = vm["wsize"].as<uint32_t>();
        conf.window_height = vm["hsize"].as<uint32_t>();
        conf.cloth_width = vm["width"].as<uint32_t>();
        conf.cloth_height = vm["height"].as<uint32_t>();
        conf.links_length = vm["linksize"].as<float>();
        conf.gravity_x = vm["gx"].as<float>();
        conf.gravity_y = vm["gy"].as<float>();
        conf.friction_coef = vm["friction"].as<float>();
        conf.disable_default_wind = vm.count("nowind") > 0;
    } catch (const po::error& err) {
        std::cerr << "failed to parse command-line: " << err.what() << std::endl;
        conf.has_error = true;
    }

    return conf;
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
