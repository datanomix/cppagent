from conan import ConanFile
from conan.tools.microsoft import is_msvc, is_msvc_static_runtime
from conan.tools.cmake import CMake, CMakeToolchain, CMakeDeps, cmake_layout
from conan.tools.files import copy

import os
import io

class MTConnectAgentConan(ConanFile):
    name = "mtconnect_agent"
    version = "2.2"
    url = "https://github.com/mtconnect/cppagent.git"
    license = "Apache License 2.0"
    settings = "os", "compiler", "arch", "build_type"
    options = { "run_tests": [True, False], "build_tests": [True, False], 
                "without_ipv6": [True, False], "with_ruby": [True, False],
                 "development" : [True, False], "shared": [True, False], "winver": [None, "ANY"],
                 "with_docs" : [True, False], "cpack": [True, False], "agent_prefix": [None, "ANY"] }
    description = "MTConnect reference C++ agent copyright Association for Manufacturing Technology"
    
    requires = ["boost/1.79.0",
                "libxml2/2.10.3",
                "date/2.4.1",
                "nlohmann_json/3.9.1",
                "openssl/3.0.8",
                "rapidjson/cci.20220822",
                "mqtt_cpp/13.1.0",
                "bzip2/1.0.8",
                "gtest/1.10.0"
                ]

    build_requires = ["cmake/[>3.23.0]"]
    
    build_policy = "missing"
    default_options = {
        "run_tests": True,
        "build_tests": True,
        "without_ipv6": False,
        "with_ruby": True,
        "development": False,
        "shared": False,
        "winver": "0x600",
        "with_docs": False,
        "cpack": False,
        "agent_prefix": None,

        "boost*:shared": False,
        "boost*:without_python": True,
        "boost*:without_test": True,
        "boost*:without_graph": True,
        "boost*:without_test": True,
        "boost*:without_nowide": True,
        "boost*:without_fiber": True,
        "boost*:without_math": True,
        "boost*:without_contract": True,
        "boost*:without_serialization": True,
        "boost*:without_wave": True,
        "boost*:without_graph_parallel": True,

        "libxml2*:shared": False,
        "libxml2*:include_utils": False,
        "libxml2*:http": False,
        "libxml2*:ftp": False,
        "libxml2*:iconv": False,
        "libxml2*:zlib": False,

        "gtest*:shared": False,

        "openssl*:shared": False,
        
        "date*:use_system_tz_db": True
        }

    run_tests = True
    build_tests = True

    exports_sources = "*"
    exports = "conan/mqtt_cpp/*", "conan/mruby/*"

    def validate(self):
        if is_msvc(self) and self.options.shared and self.settings.compiler.runtime != 'dynamic':
            raise ConanInvalidConfiguration("Shared can only be built with DLL runtime.")
        if "libcxx" in self.settings.compiler.fields and self.settings.compiler.libcxx == "libstdc++":
            raise ConanInvalidConfiguration("This package is only compatible with libstdc++11, add -s compiler.libcxx=libstdc++11")

    def layout(self):
        self.folders.build_folder_vars = ["options.shared", "settings.arch"]
        cmake_layout(self)

    def layout(self):
        self.folders.build_folder_vars = ["options.shared", "settings.arch"]
        cmake_layout(self)

    def configure(self):
        self.run_tests = self.options.run_tests
        self.build_tests = self.options.build_tests
        self.settings.compiler.cppstd = 17

        if not self.build_tests:
            self.run_tests = False

        if not self.options.shared and self.settings.os == "Macos":
            self.options["boost"].visibility = "hidden"

        # Make sure shared builds use shared boost
        if is_msvc(self) and self.options.shared:
            print("**** Making boost, libxml2, gtest, and openssl shared")
            self.options["bzip2/*"].shared = True
            self.options["boost/*"].shared = True
            self.options["libxml2/*"].shared = True
            self.options["gtest/*"].shared = True
            self.options["openssl/*"].shared = True
            
        self.run("conan export conan/mqtt_cpp", cwd=os.path.dirname(__file__))
        if self.options.with_ruby:
          self.run("conan export conan/mruby", cwd=os.path.dirname(__file__))        

    def generate(self):
        if self.options.shared:
            for dep in self.dependencies.values():
                if dep.cpp_info.bindirs:
                    if is_msvc(self) and dep.cpp_info.bindirs:
                        print("Copying from " + dep.cpp_info.bindirs[0] + " to " + os.path.join(self.build_folder, "dlls"))
                        copy(self, "*.dll", dep.cpp_info.bindirs[0], os.path.join(self.build_folder, "dlls"), keep_path=False)
                    elif dep.cpp_info.libdirs:
                        print("Copying from " + dep.cpp_info.libdirs[0] + " to " + os.path.join(self.build_folder, "dlls"))                        
                        copy(self, "*.so*", dep.cpp_info.libdirs[0], os.path.join(self.build_folder, "dlls"), keep_path=False)
                        copy(self, "*.dylib", dep.cpp_info.libdirs[0], os.path.join(self.build_folder, "dlls"), keep_path=False)            
        
        tc = CMakeToolchain(self)

        tc.cache_variables['SHARED_AGENT_LIB'] = self.options.shared.__bool__()
        tc.cache_variables['WITH_RUBY'] = self.options.with_ruby.__bool__()
        tc.cache_variables['AGENT_WITH_DOCS'] = self.options.with_docs.__bool__()
        tc.cache_variables['AGENT_ENABLE_UNITTESTS'] = self.options.build_tests.__bool__()
        tc.cache_variables['AGENT_WITHOUT_IPV6'] = self.options.without_ipv6.__bool__()
        if self.options.agent_prefix:
            tc.cache_variables['AGENT_PREFIX'] = self.options.agent_prefix
        if is_msvc(self):
            tc.cache_variables['WINVER'] = self.options.winver

        tc.generate()
        deps = CMakeDeps(self)
        deps.generate()
        
    def build_requirements(self):
        if self.options.with_docs:
            buf = io.StringIO()            
            res = self.run(["doxygen --version"], shell=True, stdout=buf)
            if (res != 0 or not buf.getvalue().startswith('1.9')):
                self.tool_requires("doxygen/1.9.4")

    def requirements(self):
        if self.options.with_ruby:
            self.requires("mruby/3.2.0")
        
    def build(self):
        cmake = CMake(self)
        cmake.verbose = True
        cmake.configure(cli_args=['--log-level=DEBUG'])
        cmake.build()
        if self.options.with_docs:
            cmake.build(build_type=None, target='docs')
        if self.run_tests:
            cmake.test()

        if self.options.cpack and self.settings.build_type == 'Release':
            print("Packaging agent with cpack")
            self.run("cpack -G ZIP", cwd=self.build_folder)

    def package_info(self):
        self.cpp_info.includedirs = ['include']
        self.cpp_info.libdirs = ['lib']
        self.cpp_info.bindirs = ['bin']
        output_name = 'agent_lib'
        if self.options.agent_prefix:
            output_name = self.options.agent_prefix + output_name
        self.cpp_info.libs = [output_name]

        self.cpp_info.defines = []
        if self.options.with_ruby:
            self.cpp_info.defines.append("WITH_RUBY=1")
        if self.options.without_ipv6:
            self.cpp_info.defines.append("AGENT_WITHOUT_IPV6=1")
        if self.options.shared:
            self.cpp_info.defines.append("SHARED_AGENT_LIB=1")
            self.cpp_info.defines.append("BOOST_ALL_DYN_LINK")
        
        if is_msvc(self):
            winver=str(self.options.winver)
            self.cpp_info.defines.append("WINVER=" + winver)
            self.cpp_info.defines.append("_WIN32_WINNT=" + winver)

    def package(self):
        cmake = CMake(self)
        cmake.install()

    
