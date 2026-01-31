cmake_minimum_required(VERSION 3.24)

project(KPulse LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable Qt's automoc/uis/rcc
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# ---- Qt6 (required) ------------------------------------------------------
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Sql DBus Svg)

# ---- KDE Frameworks 6 (optional, only for tray) -------------------------
find_package(KF6 QUIET COMPONENTS StatusNotifierItem CoreAddons)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SYSTEMD REQUIRED libsystemd)

add_subdirectory(libkpulse)
add_subdirectory(daemon)
add_subdirectory(ui)
add_subdirectory(tray)
add_subdirectory(launcher)
