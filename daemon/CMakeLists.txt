# kpulse-daemon

set(KPULSE_DAEMON_SOURCES
    src/main.cpp
    src/kpulse_daemon.cpp
    src/journald_reader.cpp
    src/metrics_collector.cpp
    src/baseline_tracker.cpp
)

qt6_add_dbus_adaptor(KPULSE_DAEMON_DBUS_ADAPTOR_SRCS
    ${CMAKE_SOURCE_DIR}/daemon/src/dbus_interface.xml
    kpulse::KPulseDaemon
    ${CMAKE_SOURCE_DIR}/daemon/src/kpulse_daemon.hpp
    kpulse_daemon_adaptor
)

add_executable(kpulse-daemon
    ${KPULSE_DAEMON_SOURCES}
    ${KPULSE_DAEMON_DBUS_ADAPTOR_SRCS}
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
