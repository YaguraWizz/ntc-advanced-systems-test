from conan import ConanFile
from conan.tools.cmake import cmake_layout


class ExampleRecipe(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeDeps", "CMakeToolchain"

    def requirements(self):
        self.requires("cppzmq/4.11.0")
        self.requires("nlohmann_json/3.11.2")
        self.requires("cli11/2.3.2")

    def layout(self):
        cmake_layout(self)