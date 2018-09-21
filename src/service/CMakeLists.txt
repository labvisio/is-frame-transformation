cmake_minimum_required(VERSION 3.10.0)

find_package(is-wire REQUIRED)
find_package(is-msgs REQUIRED)
find_package(Protobuf REQUIRED)
find_package(zipkin-cpp-opentracing REQUIRED)

get_target_property(Protobuf_IMPORT_DIRS is-msgs::is-msgs INTERFACE_INCLUDE_DIRECTORIES)
set(PROTOBUF_GENERATE_CPP_APPEND_PATH OFF)
PROTOBUF_GENERATE_CPP(options_src options_hdr conf/options.proto)

add_executable(service.bin
  service.cpp 
  calibration_server.hpp
  calibration_server.cpp
  consumer_watcher.hpp
  consumer_watcher.cpp
  dependency_tracker.hpp
  dependency_tracker.cpp
  transformation_publisher.hpp
  transformation_publisher.cpp
  ${options_src}
  ${options_hdr}
)

target_link_libraries(
  service.bin
 PUBLIC 
  is-frame-conversion::is-frame-conversion 
  is-wire::is-wire
  is-msgs::is-msgs
  zipkin-cpp-opentracing::zipkin-cpp-opentracing
)

target_include_directories(
  service.bin
 PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}> # for headers when building
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}> # for generated files in build mode
)

set_property(TARGET service.bin PROPERTY CXX_STANDARD 14)
