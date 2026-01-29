cmake_minimum_required(VERSION 3.24)

project(KPulse LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt6 is required
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Sql DBus)

# KF6 is OPTIONAL – only for the tray app
find_package(KF6 QUIET COMPONENTS StatusNotifierItem CoreAddons)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SYSTEMD REQUIRED libsystemd)

add_subdirectory(libkpulse)
add_subdirectory(daemon)
add_subdirectory(ui)

if (KF6_FOUND)
    message(STATUS "KF6 found – building kpulse-tray")
    add_subdirectory(tray)
else()
    message(STATUS "KF6 not found – skipping kpulse-tray (KDE tray app)")
endif()
