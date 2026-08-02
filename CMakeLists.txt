cmake_minimum_required(VERSION 3.10)
project(HeroStatsFinder CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Default to Release build for -O3 optimizations (crucial for scanner speed)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# Add executable
add_executable(HeroStatsFinder src/main.cpp)

# Platform specific configurations
if(MSVC)
    # Windows native MSVC
    target_compile_options(HeroStatsFinder PRIVATE /EHsc)
elseif(MINGW)
    # MinGW (Cross-compiled on Linux, or native MinGW on Windows)
    # Statically link libgcc and libstdc++ so it runs standalone on Windows/Wine
    target_link_options(HeroStatsFinder PRIVATE -static)
endif()
