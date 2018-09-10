
#include "calibration_server.hpp"
#include "boost/filesystem.hpp"
#include "boost/range.hpp"
#include "is/msgs/io.hpp"

namespace is {

auto load_calibrations(std::string const& folder) -> std::vector<vision::CameraCalibration> {
  namespace fs = boost::filesystem;
  std::vector<is::vision::CameraCalibration> calibrations;

  if (!fs::is_directory(folder)) {
    is::warn("source=CalibrationServer, event=LoadFailed, path={}, error='not a directory'",
             folder);
    return calibrations;
  }

  for (auto& entry : boost::make_iterator_range(fs::directory_iterator(folder), {})) {
    auto file = entry.path().string();
    try {
      is::vision::CameraCalibration calibration;
      is::load(file, &calibration);
      calibrations.push_back(calibration);
    } catch (std::runtime_error const& e) {
      is::warn("source=CalibrationServer, event=LoadFailed, file={}, error='{}'", file, e.what());
    }
  }
  return calibrations;
}

CalibrationServer::CalibrationServer(std::string const& path) {
  for (auto const& calibration : load_calibrations(path)) {
    _calibrations[calibration.id()] = calibration;
    is::info("source=CalibrationServer, event=NewCalibration, id={}", calibration.id());
  }
}

auto CalibrationServer::calibrations() const
    -> std::unordered_map<int64_t, vision::CameraCalibration> const& {
  return _calibrations;
}

auto CalibrationServer::get_calibration(Context*, vision::GetCalibrationRequest const& request,
                                        vision::GetCalibrationReply* reply) -> wire::Status {
  for (auto&& id : request.ids()) {
    auto it = _calibrations.find(id);
    if (it == _calibrations.end())
      return make_status(wire::StatusCode::NOT_FOUND,
                         fmt::format("CameraCalibration with id \"{}\" not found", id));
    *reply->add_calibrations() = it->second;
  }
  return make_status(wire::StatusCode::OK);
}

}  // namespace is
