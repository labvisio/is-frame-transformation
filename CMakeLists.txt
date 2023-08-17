cmake_minimum_required(VERSION 3.10.0)

set(enable_tests true CACHE BOOL "enable testing")
if (enable_tests)
  enable_testing()
endif()

set(CMAKE_EXE_LINKER_FLAGS "-Wl,--disable-new-dtags")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-rpath -Wl,.")

add_subdirectory("./src/is/frame-conversion")
add_subdirectory("./src/is/frame-conversion-service")
