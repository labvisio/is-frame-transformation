#include "utils.hpp"
#include <regex>
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

auto make_tracer(std::string const& name, std::string const& uri) -> std::shared_ptr<opentracing::Tracer> {
  std::smatch match;
  auto ok = std::regex_match(uri, match, std::regex("http:\\/\\/([a-zA-Z0-9\\.]+)(:(\\d+))?"));
  if (!ok) is::critical("Invalid zipkin uri \"{}\", expected http://<hostname>:<port>", uri);

  auto tracer_options = zipkin::ZipkinOtTracerOptions{};
  tracer_options.service_name = name;
  tracer_options.collector_host = match[1];
  tracer_options.collector_port = match[3].length() ? std::stoi(match[3]) : 9411;
  return zipkin::makeZipkinOtTracer(tracer_options);
}