from conans import ConanFile, CMake, tools


class IsFrameConversionServiceConan(ConanFile):
    name = "is-frame-conversion"
    version = "0.0.1"
    license = "MIT"
    url = "https://github.com/labviros/is-frame-conversion"
    description = ""
    settings = "os", "compiler", "build_type", "arch"
    options = {"shared": [True, False], "fPIC": [True, False], "build_tests": [True, False]}
    default_options = "shared=False", "fPIC=True", "build_tests=False"
    generators = "cmake", "cmake_find_package", "cmake_paths"
    requires = ("boost/[>=1.65]@conan/stable", "is-msgs/[>=1.1.5]@is/stable",
                "is-wire/[>=1.1.3]@is/stable", "opencv/[>=3]@is/stable",
                "zipkin-cpp-opentracing/0.3.1@is/stable")

    exports_sources = "*"

    def build_requirements(self):
        if self.options.build_tests:
            self.build_requires("gtest/1.8.0@bincrafters/stable")

    def configure(self):
        self.options["is-msgs"].shared = True
        self.options["opencv"].with_qt = False
        self.options["spdlog"].fmt_external = False
        if self.options.shared:
            self.options["boost"].fPIC = True

    def build(self):
        cmake = CMake(self, generator='Ninja')
        cmake.definitions["CMAKE_POSITION_INDEPENDENT_CODE"] = self.options.fPIC
        cmake.definitions["enable_tests"] = self.options.build_tests
        cmake.configure()
        cmake.build()
        if self.options.build_tests:
            cmake.test()
        cmake.install()

    def package_info(self):
        self.cpp_info.libs = ["is-frame-conversion"]
