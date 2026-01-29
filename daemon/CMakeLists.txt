add_executable(kpulse-daemon
    src/main.cpp
    src/kpulse_daemon.cpp
    src/journald_reader.cpp
    src/metrics_collector.cpp
)

target_include_directories(kpulse-daemon
    PRIVATE
        ${SYSTEMD_INCLUDE_DIRS}
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(kpulse-daemon
    PRIVATE
        kpulse
        Qt6::Core
        Qt6::DBus
        ${SYSTEMD_LIBRARIES}
)
