/* Configuration management */

/* JSON configuration format:
{
  "size": Vector2<int>,
  "length": links_length,
  "friction": friction_coef,
  "gravity": Vector2<float>,
  "wind": [
    []
  ],
  "structure": {
    "nodes": [],
    "pins": []
  }
}

Vectors can be specified one of two ways: [x, y] and {"x": x, "y": y}

 */

#include <sstream>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <boost/program_options.hpp>
#include <nlohmann/json.hpp>
#include <cerrno>

#include "engine/window_context_handler.hpp"
#include "engine/physics/physics.hpp"
#include "renderer.hpp"
#include "wind.hpp"

namespace po = boost::program_options;
using json = nlohmann::json;

const uint32_t WINDOW_WIDTH_DEFAULT = 1920;
const uint32_t WINDOW_HEIGHT_DEFAULT = 1080;
const uint32_t CLOTH_WIDTH_DEFAULT = 75;
const uint32_t CLOTH_HEIGHT_DEFAULT = 50;
const float LINKS_LENGTH_DEFAULT = 20.0f;
const float ERASE_RADIUS_DEFAULT = 10.0f;
const float MOUSE_RADIUS_DEFAULT = 100.0f;
const float MOUSE_FORCE_DEFAULT = 8000.0f;

/* Struct for maintaining command-line arguments */
struct config {
    enum Status {
        OK = 0,
        EXIT = 1,
        ERROR = 2
    };
    config()
        : debug(false)
        , window_width(WINDOW_WIDTH_DEFAULT)
        , window_height(WINDOW_HEIGHT_DEFAULT)
        , cloth_width(CLOTH_WIDTH_DEFAULT)
        , cloth_height(CLOTH_HEIGHT_DEFAULT)
        , links_length(LINKS_LENGTH_DEFAULT)
        , gravity_x(GRAVITY_X_DEFAULT)
        , gravity_y(GRAVITY_Y_DEFAULT)
        , friction_coef(FRICTION_DEFAULT)
        , disable_default_wind(false)
        , erase_radius(ERASE_RADIUS_DEFAULT)
        , mouse_drag_radius(MOUSE_RADIUS_DEFAULT)
        , mouse_drag_force(MOUSE_FORCE_DEFAULT)
        , initial_zoom(BASE_ZOOM_DEFAULT)
        , cloth_definition_path()
    {}
    /* command-line variables */
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
    float erase_radius;
    float mouse_drag_radius;
    float mouse_drag_force;
    float initial_zoom;
    std::string cloth_definition_path;
    std::vector<Wind> winds;

    /* Parse command-line arguments and return a status; 0 = success */
    Status parseCommandLineArguments(int argc, char* argv[]);

    /* Parse a JSON configuration file and return a status; 0 = success */
    Status parseConfigurationFile(const std::string& fpath);

    /* Build the cloth based on the current configuration */
    void buildCloth(PhysicSolver& solver) const;

    /* Dump the current values to the given ostream */
    void print(std::ostream& os) const;

private:
    /* Extract values from the given json object */
    Status interpretJSON(const json& jobj);

    /* Interpret a value as a V2f */
    template <typename T>
    sf::Vector2<T> interpretVecJSON(const json& item) const;

    /* Interpret value as a V2f with nulls interpreted as defaults */
    template <typename T>
    sf::Vector2<T> interpretVecJSON(const json& item, const T& xdef, const T& ydef) const;
};

/* vim: set ts=4 sts=4 sw=4 et: */
