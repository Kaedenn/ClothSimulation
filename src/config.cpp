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
        cloth_definition_path = vm["defpath"].as<std::string>();
        if (cloth_definition_path.length() > 0) {
            status = parseConfigurationFile(cloth_definition_path);
        }
    } catch (const po::error& err) {
        std::cerr << "failed to parse command-line: " << err.what() << std::endl;
        status = config::Status::ERROR;
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
        throw;
        return Status::ERROR;
    }
    return Status::OK;
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
    << "cloth definition file: " << cloth_definition_path << std::endl;
}

Status config::interpretJSON(const json& jobj)
{
    if (jobj.contains("size") && jobj["size"].is_array()) {
        cloth_width = jobj["/size/0"_json_pointer];
        cloth_height = jobj["/size/1"_json_pointer];
    }
    if (jobj.contains("length")) {
        links_length = jobj["length"];
    }
    if (jobj.contains("friction")) {
        friction_coef = jobj["friction"];
    }
    if (jobj.contains("gravity") && jobj["gravity"].is_array()) {
        gravity_x = jobj["/gravity/0"_json_pointer];
        gravity_y = jobj["/gravity/1"_json_pointer];
    }
    return Status::OK;
}

