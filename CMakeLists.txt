cmake_minimum_required(VERSION 3.24)

project(KPulse LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(KPULSE_DEV_MODE "Developer mode (relaxed dependency checks)" OFF)

# ---- Qt6 discovery -------------------------------------------------------

set(KPULSE_QT6_REQUIRED_COMPONENTS
    Core
    Widgets
    DBus
    Sql
)

find_package(Qt6 QUIET COMPONENTS ${KPULSE_QT6_REQUIRED_COMPONENTS})

if (NOT Qt6_FOUND)
    message(STATUS "")
    message(STATUS "============================================================")
    message(STATUS " Qt6 was NOT found.")
    message(STATUS "")
    message(STATUS " KPulse requires Qt6 development packages to build.")
    message(STATUS "")
    message(STATUS " On Arch / CachyOS:")
    message(STATUS "   sudo pacman -S qt6-base qt6-tools qt6-declarative")
    message(STATUS "")
    message(STATUS " Or configure manually:")
    message(STATUS "   cmake -DQt6_DIR=/usr/lib/cmake/Qt6 ..")
    message(STATUS "")
    message(STATUS " Configuration will stop here.")
    message(STATUS "============================================================")
    message(STATUS "")
    if (NOT KPULSE_DEV_MODE)
        return()
    endif()
    message(STATUS "KPULSE_DEV_MODE=ON: skipping target configuration without Qt6.")
    return()
endif()

find_package(KF6 QUIET COMPONENTS
    StatusNotifierItem
    CoreAddons
)

if (NOT KF6_FOUND)
    message(STATUS "")
    message(STATUS "KF6 (KDE Frameworks 6) not found.")
    message(STATUS "Tray and KDE integration will not be built.")
    message(STATUS "")
endif()
find_package(PkgConfig REQUIRED)

pkg_check_modules(SYSTEMD REQUIRED libsystemd)

add_subdirectory(libkpulse)
add_subdirectory(daemon)
add_subdirectory(ui)

if (KF6_FOUND)
    add_subdirectory(tray)
else()
    message(STATUS "Skipping kpulse-tray (KF6 not available)")
endif()

include(GNUInstallDirs)

install(TARGETS kpulse-daemon kpulse-ui kpulse-tray
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
