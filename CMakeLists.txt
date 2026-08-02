cmake_minimum_required(VERSION 3.10)
project(TaskBarHero-GodMode CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Default to Release build for -O3 optimizations (crucial for scanner speed)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# Executable name
set(EXE_NAME "TBH-GodMode")

# Add executable
add_executable(${EXE_NAME} src/main.cpp)

# Platform specific configurations
if(MSVC)
    # Windows native MSVC
    target_compile_options(${EXE_NAME} PRIVATE /EHsc)
elseif(MINGW)
    # MinGW (Cross-compiled on Linux, or native MinGW on Windows)
    # Statically link libgcc and libstdc++ so it runs standalone on Windows/Wine
    target_link_options(${EXE_NAME} PRIVATE -static)
endif()

# Wrapper Scripts Generation
configure_file(scripts/launch.bat.in launch.bat @ONLY)
configure_file(scripts/launch_linux.sh.in launch_linux.sh @ONLY)
configure_file(scripts/launch_macos.sh.in launch_macos.sh @ONLY)

# --- CPack Configuration for Release Packaging ---
include(InstallRequiredSystemLibraries)
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/License")
set(CPACK_PACKAGE_VERSION_MAJOR "1")
set(CPACK_PACKAGE_VERSION_MINOR "0")
set(CPACK_PACKAGE_VERSION_PATCH "0")
set(CPACK_PACKAGE_FILE_NAME "TBH-GodMode-Release")
set(CPACK_GENERATOR "ZIP")

# Install instructions (CPack uses these to know what to zip)
install(TARGETS ${EXE_NAME} DESTINATION .)
install(FILES 
    "${CMAKE_BINARY_DIR}/launch.bat"
    "${CMAKE_SOURCE_DIR}/README.md"
    DESTINATION .
)
install(PROGRAMS 
    "${CMAKE_BINARY_DIR}/launch_linux.sh"
    "${CMAKE_BINARY_DIR}/launch_macos.sh"
    DESTINATION .
)

include(CPack)
