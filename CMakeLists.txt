cmake_minimum_required(VERSION 3.10.0)

include(${CMAKE_BINARY_DIR}/conan_paths.cmake)

set(enable_tests true CACHE BOOL "enable testing")
if (enable_tests)
  enable_testing()
endif()

add_subdirectory("./frame_conversion")

find_package(is-wire REQUIRED)
find_package(Protobuf REQUIRED)

get_target_property(Protobuf_IMPORT_DIRS is-msgs::is-msgs INTERFACE_INCLUDE_DIRECTORIES)
set(PROTOBUF_GENERATE_CPP_APPEND_PATH OFF)
PROTOBUF_GENERATE_CPP(options_src options_hdr options.proto)

add_executable(service
  service/service.cpp 
  service/utils.hpp 
  service/utils.cpp 
  service/consumer_watcher.hpp
  service/consumer_watcher.cpp
  service/dependency_tracker.hpp
  service/dependency_tracker.cpp
  ${options_src}
  ${options_hdr}
)

target_link_libraries(
  service 
 PUBLIC 
  is-frame-conversion::is-frame-conversion 
  is-wire::is-wire
)

target_include_directories(
  service
 PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}> # for headers when building
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}> # for generated files in build mode
)

set_property(TARGET service PROPERTY CXX_STANDARD 11)