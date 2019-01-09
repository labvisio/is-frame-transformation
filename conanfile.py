from conans import ConanFile, CMake, tools


class IsFrameTransformationServiceConan(ConanFile):
    name = "is-frame-transformation"
    version = "0.0.3"
    license = "MIT"
    url = "https://github.com/labviros/is-frame-transformation"
    description = ""
    settings = "os", "compiler", "build_type", "arch"
    options = {
        "shared": [True, False],
        "fPIC": [True, False],
        "build_tests": [True, False],
    }
    default_options = "shared=False", "fPIC=True", "build_tests=False"
    generators = "cmake", "cmake_find_package", "cmake_paths", "virtualrunenv"
    requires = (
        "is-msgs/1.1.10@is/stable",
        "is-wire/1.1.4@is/stable",
        "opencv/3.4.2@is/stable",
        "zipkin-cpp-opentracing/0.3.1@is/stable",
        "expected/0.3.0@is/stable",
        "boost/1.66.0@conan/stable",
    )

    exports_sources = "*"

    def build_requirements(self):
        if self.options.build_tests:
            self.build_requires("gtest/1.8.0@bincrafters/stable")

    def configure(self):
        self.options["is-msgs"].shared = True
        self.options["opencv"].with_qt = False

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
