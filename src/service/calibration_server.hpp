#pragma once

#include <unordered_map>
#include "is/msgs/camera.pb.h"
#include "is/wire/rpc.hpp"

namespace is {

class CalibrationServer {
  std::unordered_map<int64_t, vision::CameraCalibration> _calibrations;

 public:
  CalibrationServer(std::string const& path);

  auto calibrations() const -> std::unordered_map<int64_t, vision::CameraCalibration> const&;

  auto get_calibration(Context*, vision::GetCalibrationRequest const& request,
                       vision::GetCalibrationReply* reply) -> wire::Status;
};

}  // namespace is