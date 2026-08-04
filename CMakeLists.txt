cmake_minimum_required(VERSION 3.16)
project(ReplayQt CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)

# Default version if none is specified
if(NOT DEFINED APP_VERSION)
    set(APP_VERSION "v0.1" CACHE STRING "App Version")
endif()

# Injects APP_VERSION directly into C++ compiler definitions
add_compile_definitions(APP_VERSION="${APP_VERSION}")

find_package(Qt6 COMPONENTS Widgets QUIET)
if (Qt6_FOUND)
    set(QT_LIBRARIES Qt6::Widgets)
else()
    find_package(Qt5 COMPONENTS Widgets REQUIRED)
    set(QT_LIBRARIES Qt5::Widgets)
endif()

add_executable(ReplayQt WIN32 main.cpp app.rc)

if (MINGW)
    target_link_options(ReplayQt PRIVATE -static-libgcc -static-libstdc++)
endif()

target_link_libraries(ReplayQt PRIVATE ${QT_LIBRARIES} user32)
