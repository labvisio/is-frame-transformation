cmake_minimum_required(VERSION 3.10.0)

include(${CMAKE_BINARY_DIR}/conan_paths.cmake)

set(enable_tests true CACHE BOOL "enable testing")
if (enable_tests)
  enable_testing()
endif()

add_subdirectory("./frame_conversion")

find_package(is-wire REQUIRED)
find_package(Protobuf REQUIRED)
find_package(zipkin-cpp-opentracing)

get_target_property(Protobuf_IMPORT_DIRS is-msgs::is-msgs INTERFACE_INCLUDE_DIRECTORIES)
set(PROTOBUF_GENERATE_CPP_APPEND_PATH OFF)
PROTOBUF_GENERATE_CPP(options_src options_hdr options.proto)

add_executable(service.bin
  service/service.cpp 
  service/utils.hpp 
  service/utils.cpp 
  service/calibration_server.hpp
  service/calibration_server.cpp
  service/consumer_watcher.hpp
  service/consumer_watcher.cpp
  service/dependency_tracker.hpp
  service/dependency_tracker.cpp
  service/transformation_publisher.hpp
  service/transformation_publisher.cpp
  ${options_src}
  ${options_hdr}
)

target_link_libraries(
  service.bin
 PUBLIC 
  is-frame-conversion::is-frame-conversion 
  is-wire::is-wire
  zipkin-cpp-opentracing::zipkin-cpp-opentracing
)

target_include_directories(
  service.bin
 PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}> # for headers when building
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}> # for generated files in build mode
)

set_property(TARGET service.bin PROPERTY CXX_STANDARD 14)
