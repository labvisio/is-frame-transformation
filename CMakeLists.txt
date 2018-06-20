cmake_minimum_required(VERSION 3.10.0)
include(GNUInstallDirs)

include(${CMAKE_BINARY_DIR}/conan_paths.cmake)

find_package(boost REQUIRED)

add_executable(main main.cpp)
target_link_libraries(main boost::boost)
set_property(TARGET main PROPERTY CXX_STANDARD 11)