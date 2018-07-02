#include "utils.hpp"
#include "boost/filesystem.hpp"
#include "boost/range.hpp"
#include "is/wire/core/logger.hpp"

auto load_calibrations(std::string const& folder) -> std::vector<is::vision::CameraCalibration> {
  namespace fs = boost::filesystem;
  std::vector<is::vision::CameraCalibration> calibrations;

  if (!fs::is_directory(folder)) {
    is::warn("Unable to load calibrations, path \"{}\" is not a directory", folder);
    return calibrations;
  }

  for (auto& entry : boost::make_iterator_range(fs::directory_iterator(folder), {})) {
    is::vision::CameraCalibration calibration;
    auto status = is::load(entry.path().string(), &calibration);
    if (status.code() == is::wire::StatusCode::OK) {
      calibrations.push_back(calibration);
    } else {
      is::warn("Failed to load file: {}", status.why());
    }
  }
  return calibrations;
}

auto load_cli_options(int argc, char** argv) -> is::FrameConversionServiceOptions {
  std::string filename = (argc == 2) ? argv[1] : "options.json";
  auto leave_on_error = [](is::wire::Status const& status) {
    if (status.code() != is::wire::StatusCode::OK) is::critical("{}", status);
  };
  is::FrameConversionServiceOptions options;
  leave_on_error(is::load(filename, &options));
  leave_on_error(is::validate_message(options));
  return options;
}