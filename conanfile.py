from conan import ConanFile
from conan.tools.cmake import CMake, cmake_layout


class LockablesConan(ConanFile):
    name = "lockables"
    description = "Lockables"
    homepage = "https://github.com/luketokheim/lockables"
    license = "BSL-1.0"

    settings = "os", "arch", "compiler", "build_type"
    generators = "CMakeToolchain", "CMakeDeps", "VirtualRunEnv"
    exports_sources = "CMakeLists.txt", "cmake/*", "include/*"
    no_copy_source = True

    options = {
        "developer_mode": [True, False],
        "enable_benchmarks": [True, False]
    }
    default_options = {"developer_mode": False, "enable_benchmarks": False}

    def build_requirements(self):
        if not self.options.developer_mode:
            return

        if self.options.enable_benchmarks:
            self.test_requires("benchmark/1.7.1")

        if not self.conf.get("tools.build:skip_test", default=False):
            self.test_requires("catch2/3.3.2")

    def layout(self):
        cmake_layout(self)

    def build(self):
        variables = dict()
        if self.options.developer_mode:
            variables["lockables_DEVELOPER_MODE"] = True
        if self.options.enable_benchmarks:
            variables["ENABLE_BENCHMARKS"] = True

        cmake = CMake(self)
        cmake.configure(variables=variables)
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()
