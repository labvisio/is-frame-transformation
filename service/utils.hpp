#pragma once

#include <string>
#include <vector>
#include "is/msgs/camera.pb.h"
#include "is/msgs/io.hpp"
#include "options.pb.h"

auto load_calibrations(std::string const& folder) -> std::vector<is::vision::CameraCalibration>;
auto load_cli_options(int argc, char** argv) -> is::FrameConversionServiceOptions;