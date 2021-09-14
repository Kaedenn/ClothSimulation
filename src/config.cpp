/* Source file implementing include/config.hpp */

#include "config.hpp"

using Status = config::Status;

/* Parse command-line arguments and return a status; 0 = success */
Status config::parseCommandLineArguments(int argc, char* argv[])
{
    config::Status status = config::Status::OK;
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
        ("zoom,Z", po::value<float>()->default_value(BASE_ZOOM_DEFAULT),
        "initial zoom amount")
        ("defpath,P", po::value<std::string>(),
        "path to optional cloth definition JSON file")
        ;
    opts.add(phys_opts);
    try {
        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, opts), vm);
        vm.notify();
        if (vm.count("help")) {
            std::cerr << "usage: " << argv[0] << " [options...]" << std::endl;
            opts.print(std::cerr);
            std::cerr << "\nkeyboard controls:"
                << "\n  " << std::left << std::setw(16) << "Escape" << "close program"
                << "\n  " << std::left << std::setw(16) << "Space" << "toggle wind"
                << "\n  " << std::left << std::setw(16) << "/" << "output viewport state"
                << std::endl;
            status = config::Status::EXIT;
        }
        debug = vm.count("verbose") > 0;
        window_width = vm["wsize"].as<uint32_t>();
        window_height = vm["hsize"].as<uint32_t>();
        cloth_width = vm["width"].as<uint32_t>();
        cloth_height = vm["height"].as<uint32_t>();
        links_length = vm["linksize"].as<float>();
        gravity_x = vm["gx"].as<float>();
        gravity_y = vm["gy"].as<float>();
        friction_coef = vm["friction"].as<float>();
        disable_default_wind = vm.count("nowind") > 0;
        initial_zoom = vm["zoom"].as<float>();
        if (vm.count("defpath") > 0) {
            cloth_definition_path = vm["defpath"].as<std::string>();
            if (cloth_definition_path.length() > 0) {
                status = parseConfigurationFile(cloth_definition_path);
            }
        }
    } catch (const po::error& err) {
        std::cerr << "failed to parse command-line: " << err.what() << std::endl;
        status = config::Status::ERROR;
#ifndef NDEBUG
        throw;
#endif
    } catch (const json::exception& err) {
        std::cerr << "failed to parse configuration: " << err.what() << std::endl;
        status = config::Status::ERROR;
#ifndef NDEBUG
        throw;
#endif
    } catch (const std::logic_error& err) {
        std::cerr << "failed to parse configuration: " << err.what() << std::endl;
        status = config::Status::ERROR;
#ifndef NDEBUG
        throw;
#endif
    }

    return status;
}

/* Parse a JSON configuration file and return a status; 0 = success */
Status config::parseConfigurationFile(const std::string& fpath)
{
    if (debug) std::cerr << "Parsing JSON " << fpath << std::endl;
    std::ifstream ifs(fpath);
    if (!ifs) {
        std::cerr << "Failed reading " << fpath << ": error " << errno
            << " " << std::strerror(errno) << std::endl;
        return Status::ERROR;
    }
    try {
        json jobj = json::parse(ifs);
        if (debug) std::cerr << "Parsed JSON: " << jobj << std::endl;
        return interpretJSON(jobj);
    }
    catch (const json::parse_error& e) {
        std::cerr << "Failed to parse " << fpath << ": " << e.what() << std::endl;
#ifndef NDEBUG
        throw;
#else
        return Status::ERROR;
#endif
    }
    return Status::OK;
}

void config::buildCloth(PhysicSolver& solver) const
{
    const float start_x = (window_width - (cloth_width - 1) * links_length) * 0.5;
    for (uint32_t y = 0; y < cloth_height; ++y) {
        const float max_elongation = 1.2f * (2.0f - y / float(cloth_height));
        for (uint32_t x = 0; x < cloth_width; ++x) {
            const auto ppos = sf::Vector2f(start_x + x * links_length, y * links_length);
            const civ::ID id = solver.addParticle(ppos);
            if (x > 0) {
                solver.addLink(id-1, id, max_elongation * 0.9f);
            }
            if (y > 0) {
                solver.addLink(id-cloth_width, id, max_elongation);
            } else {
                solver.objects[id].moving = false;
            }
        }
    }
}

void config::print(std::ostream& os) const
{
    os << "configuration:" << "\n"
       << "verbose: " << debug << "\n"
       << "window size: " << window_width << " by " << window_height << "\n"
       << "cloth size: " << cloth_width << " by " << cloth_height << "\n"
       << "link length: " << links_length << "\n"
       << "gravity vector: " << gravity_x << "," << gravity_y << "\n"
       << "friction coefficient: " << friction_coef << "\n"
       << "default wind: " << (disable_default_wind ? "disabled" : "enabled") << "\n"
       << "mouse erase radius: " << erase_radius << "\n"
       << "mouse drag radius: " << mouse_drag_radius << "\n"
       << "mouse drag force: " << mouse_drag_force << "\n"
       << "cloth definition file: " << cloth_definition_path << "\n";
    for (uint32_t i = 0; i < winds.size(); ++i) {
        const Wind& wind = winds[i];
        os << "wind " << i+1
           << " [[" << wind.rect.left << "," << wind.rect.top << "], ["
           << wind.rect.left + wind.rect.width << ","
           << wind.rect.top + wind.rect.height << "]]; force: "
           << wind.force.x << ", " << wind.force.y << std::endl;
    }
}

/** Interpret a JSON object and update the configuration accordingly */
Status config::interpretJSON(const json& jobj)
{
    if (jobj.contains("size")) {
        const sf::Vector2<int> size = interpretVec2JSON<int>(jobj["size"]);
        cloth_width = size.x;
        cloth_height = size.y;
    }
    if (jobj.contains("length")) {
        links_length = jobj["length"];
    }
    if (jobj.contains("friction")) {
        friction_coef = jobj["friction"];
    }
    if (jobj.contains("gravity")) {
        const sf::Vector2f gravity = interpretVec2JSON<float>(jobj["gravity"]);
        gravity_x = gravity.x;
        gravity_y = gravity.y;
    }
    if (jobj.contains("wind")) {
        for (auto item : jobj["wind"]) {
            if (!item.is_array() || item.size() != 3) {
                throw std::logic_error("Failed to parse wind " + std::string(item) + "; not an array of size 3");
            }
            const sf::Vector2f wind_s = interpretVec2JSON<float>(item.at(0), 0.0f, to<float>(window_height));
            const sf::Vector2f wind_p = interpretVec2JSON<float>(item.at(1), 0.0f, 0.0f);
            const sf::Vector2f wind_f = interpretVec2JSON<float>(item.at(2));
            winds.emplace_back(wind_s, wind_p, wind_f);
        }
    }
    return Status::OK;
}

/** Interpret the JSON object as a vector of two items */
template <typename T>
sf::Vector2<T> config::interpretVec2JSON(const json& item) const
{
    if (item.is_array() && item.size() == 2) {
        const T& v0 = item["/0"_json_pointer];
        const T& v1 = item["/1"_json_pointer];
        return sf::Vector2<T>(v0, v1);
    } else if (item.is_object() && item.contains("x") && item.contains("y")) {
        const T& v0 = item["/x"_json_pointer];
        const T& v1 = item["/y"_json_pointer];
        return sf::Vector2<T>(v0, v1);
    }
    throw std::logic_error("Failed to parse Vector2 from JSON: " + std::string(item));
}

/** Interpret the JSON object as a vector of two items with defaults */
template <typename T>
sf::Vector2<T> config::interpretVec2JSON(const json& item, const T& v0def, const T& v1def) const
{
    json::json_pointer p0, p1;
    if (item.is_array() && item.size() == 2) {
        p0 = json::json_pointer("/0");
        p1 = json::json_pointer("/1");
    } else if (item.is_object() && item.contains("x") && item.contains("y")) {
        p0 = json::json_pointer("/x");
        p1 = json::json_pointer("/y");
    } else {
        throw std::logic_error("Failed to parse Vector2 from JSON: " + std::string(item));
    }
    const json& i0 = item[p0];
    const json& i1 = item[p1];
    const T& v0 = i0.is_null() ? v0def : to<T>(i0);
    const T& v1 = i1.is_null() ? v1def : to<T>(i1);
    return sf::Vector2<T>(v0, v1);
}

/* vim: set ts=4 sts=4 sw=4 et: */
