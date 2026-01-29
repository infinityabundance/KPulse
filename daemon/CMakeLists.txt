add_executable(kpulse-daemon
    src/main.cpp
)

target_include_directories(kpulse-daemon
    PRIVATE
        ${SYSTEMD_INCLUDE_DIRS}
)

target_link_libraries(kpulse-daemon
    PRIVATE
        kpulse
        Qt6::Core
        Qt6::DBus
        ${SYSTEMD_LIBRARIES}
)
