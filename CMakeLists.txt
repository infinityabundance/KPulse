cmake_minimum_required(VERSION 3.24)

project(KPulse LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Sql DBus)
find_package(KF6 REQUIRED COMPONENTS StatusNotifierItem CoreAddons)
find_package(PkgConfig REQUIRED)

pkg_check_modules(SYSTEMD REQUIRED libsystemd)

add_subdirectory(libkpulse)
add_subdirectory(daemon)
add_subdirectory(ui)
add_subdirectory(tray)
