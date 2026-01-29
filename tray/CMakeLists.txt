add_executable(kpulse-tray
    src/main.cpp
    src/tray_app.cpp
)

target_include_directories(kpulse-tray
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(kpulse-tray
    PRIVATE
        kpulse
        Qt6::Core
        Qt6::Widgets
        KF6::StatusNotifierItem
        KF6::CoreAddons
)
