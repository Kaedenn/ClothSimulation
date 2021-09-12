# CMake script to create the Makefiles necessary to build this project

cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

set(PROJECT_NAME Cloth)
set(PROJECT_VERSION 2.0.0)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake_modules" ${CMAKE_MODULE_PATH})
project(${PROJECT_NAME} VERSION ${PROJECT_VERSION} LANGUAGES CXX)

# Set build type
if(NOT CMAKE_BUILD_TYPE)
  if($ENV{CMAKE_BUILD_TYPE})
    set(CMAKE_BUILD_TYPE $ENV{CMAKE_BUILD_TYPE})
  else()
    set(CMAKE_BUILD_TYPE Release)
  endif()
endif(NOT CMAKE_BUILD_TYPE)

# Include dependent module(s)
include(FetchContent)

# Find and add source files
file(GLOB_RECURSE source_files
  "src/*.cpp"
  "include/*.hpp")

set(SOURCES ${source_files})

add_executable(${PROJECT_NAME} ${WIN32_GUI} ${SOURCES})
target_include_directories(${PROJECT_NAME} PRIVATE "include" "lib")
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 17)

# Detect and add SFML
find_package(SFML 2 COMPONENTS network audio graphics window system REQUIRED)
set(SFML_LIBS sfml-system sfml-window sfml-graphics)
target_link_libraries(${PROJECT_NAME} ${SFML_LIBS})

# Detect and add libboost
find_package(Boost COMPONENTS program_options REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} ${Boost_LIBRARIES})

# Download and add nlohmann/json
FetchContent_Declare(json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.10.2)
FetchContent_GetProperties(json)
if(NOT json_POPULATED)
  FetchContent_Populate(json)
  add_subdirectory(${json_SOURCE_DIR} ${json_BINARY_DIR} EXCLUDE_FROM_ALL)
endif(NOT json_POPULATED)

target_link_libraries(${PROJECT_NAME} nlohmann_json::nlohmann_json)

# *nix: add libpthread
if(UNIX)
  target_link_libraries(${PROJECT_NAME} pthread)
endif(UNIX)

# Set compile options
if(MSVC)
  target_compile_options(${PROJECT_NAME} PRIVATE /W4 /WX)
else()
  target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

# For MSVC, copy the libraries to the lib directory
if(MSVC)
  foreach(lib ${SFML_LIBS})
    get_target_property(lib_path ${lib} LOCATION)
    file(COPY ${lib_path} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
  endforeach()
  foreach(lib ${Boost_LIBS})
    get_target_property(lib_path ${lib} LOCATION)
    file(COPY ${lib_path} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
  endforeach()
endif(MSVC)

# Display debugging information if required
if ($ENV{DEBUG})
  message("CMake configuration complete; results:")
  message("Build type: ${CMAKE_BUILD_TYPE}")
  message("CMAKE_LIBRARY_PATH: ${CMAKE_LIBRARY_PATH}")
  message("CMAKE_MODULE_PATH: ${CMAKE_MODULE_PATH}")
  message("CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")
  message("CMAKE_CXX_FLAGS_DEBUG: ${CMAKE_CXX_FLAGS_DEBUG}")
  message("CMAKE_CXX_FLAGS_RELEASE: ${CMAKE_CXX_FLAGS_RELEASE}")
endif()
