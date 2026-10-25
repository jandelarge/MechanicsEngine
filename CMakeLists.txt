cmake_minimum_required(VERSION 3.15)
project(MechanicsEngine CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
        raylib
        URL https://github.com/raysan5/raylib/archive/refs/tags/5.5.tar.gz
)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(raylib)

file(GLOB_RECURSE SOURCES "src/*.cpp")

add_executable(MechanicsEngine ${SOURCES})
target_include_directories(MechanicsEngine PRIVATE src)
target_link_libraries(MechanicsEngine PRIVATE raylib)

if (MSVC)
    target_compile_options(MechanicsEngine PRIVATE /W0)
endif()

if (MINGW)
    target_link_options(MechanicsEngine PRIVATE -static -static-libgcc -static-libstdc++ -mwindows)
endif()