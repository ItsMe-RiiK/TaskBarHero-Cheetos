cmake_minimum_required(VERSION 3.10)
project(TBH_Cheetos VERSION 1.0.1 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Default to Release build for -O3 optimizations (crucial for scanner speed)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# Executable name
set(EXE_NAME "TBH-Cheetos")

# Add executable
add_executable(${EXE_NAME} 
    src/main.cpp
    src/features/RuneUnlocker.cpp
    src/scanner/PlayerDataFinder.cpp
    src/ui/CheetosGUI.cpp
    resources/resources.rc
)
if(NOT APP_VERSION)
    set(APP_VERSION "v${PROJECT_VERSION}")
endif()

target_include_directories(${EXE_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_compile_definitions(${EXE_NAME} PRIVATE APP_VERSION="${APP_VERSION}")
target_link_options(${EXE_NAME} PRIVATE "-mwindows")

# UI Dependencies
find_package(imgui CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
target_link_libraries(${EXE_NAME} PRIVATE imgui::imgui glfw opengl32)

# Speedhack DLL target
find_package(minhook CONFIG REQUIRED)

add_library(speedhack SHARED 
    src/features/speedhack.cpp
)
target_link_libraries(speedhack PRIVATE minhook::minhook)
set_target_properties(speedhack PROPERTIES PREFIX "")

# Platform specific configurations
if(MSVC)
    # Windows native MSVC
    target_compile_options(${EXE_NAME} PRIVATE /EHsc)
    target_link_libraries(${EXE_NAME} PRIVATE comctl32 psapi)
elseif(MINGW)
    # MinGW (Cross-compiled on Linux, or native MinGW on Windows)
    # Statically link libgcc and libstdc++ so it runs standalone on Windows/Wine
    target_link_options(${EXE_NAME} PRIVATE -static)
    target_link_libraries(${EXE_NAME} PRIVATE comctl32 psapi)
endif()

# Wrapper Scripts Generation
configure_file(resources/templates/README.txt.in README.txt @ONLY)

# --- CPack Configuration for Release Packaging ---
include(InstallRequiredSystemLibraries)
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/license")
set(CPACK_PACKAGE_VERSION_MAJOR "2")
set(CPACK_PACKAGE_VERSION_MINOR "0")
set(CPACK_PACKAGE_VERSION_PATCH "0")
set(CPACK_PACKAGE_FILE_NAME "TBH-Cheetos-Release")
set(CPACK_GENERATOR "ZIP")

# Install instructions (CPack uses these to know what to zip)
install(TARGETS ${EXE_NAME} speedhack RUNTIME DESTINATION .)
install(FILES 
    "${CMAKE_SOURCE_DIR}/scripts/launch.bat"
    "${CMAKE_BINARY_DIR}/README.txt"
    DESTINATION .
)
install(PROGRAMS 
    "${CMAKE_SOURCE_DIR}/scripts/launch_linux.sh"
    "${CMAKE_SOURCE_DIR}/scripts/launch_macos.sh"
    DESTINATION .
)

include(CPack)
