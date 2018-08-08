
#include "calibration_server.hpp"
#include "boost/filesystem.hpp"
#include "boost/range.hpp"
#include "is/msgs/io.hpp"

namespace is {

auto load_calibrations(std::string const& folder) -> std::vector<vision::CameraCalibration> {
  namespace fs = boost::filesystem;
  std::vector<is::vision::CameraCalibration> calibrations;

  if (!fs::is_directory(folder)) {
    is::warn("[LoadCalibrations] Unable to load calibrations, path \"{}\" is not a directory",
             folder);
    return calibrations;
  }

  for (auto& entry : boost::make_iterator_range(fs::directory_iterator(folder), {})) {
    is::vision::CameraCalibration calibration;
    auto status = is::load(entry.path().string(), &calibration);
    if (status.code() == is::wire::StatusCode::OK) {
      calibrations.push_back(calibration);
    } else {
      is::warn("[LoadCalibrations] Failed to load file: {}", status.why());
    }
  }
  return calibrations;
}

CalibrationServer::CalibrationServer(std::string const& path) {
  for (auto const& calibration : load_calibrations(path)) {
    _calibrations[calibration.id()] = calibration;
    is::info("[CalibrationServer][+] id={}", calibration.id());
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
