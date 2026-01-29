add_executable(kpulse-tray
    src/main.cpp
)

target_link_libraries(kpulse-tray
    PRIVATE
        kpulse
        Qt6::Core
        Qt6::Widgets
        KF6::StatusNotifierItem
        KF6::CoreAddons
)
