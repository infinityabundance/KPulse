# KPulse daemon: journald + metrics collector + DBus service

set(KPULSE_DAEMON_SOURCES
    src/main.cpp
    src/kpulse_daemon.cpp
    src/journald_reader.cpp
    src/metrics_collector.cpp
)

# Generate DBus adaptor for kpulse::KPulseDaemon from the XML interface.
# Correct argument order for qt6_add_dbus_adaptor is:
#   qt6_add_dbus_adaptor(<out_var>
#                        <xml_file>
#                        <header_with_class>
#                        <classname>
#                        [basename])
qt6_add_dbus_adaptor(KPULSE_DAEMON_DBUS_ADAPTOR_SRCS
    ${CMAKE_SOURCE_DIR}/daemon/src/dbus_interface.xml
    ${CMAKE_SOURCE_DIR}/daemon/src/kpulse_daemon.hpp
    kpulse::KPulseDaemon
    kpulse_daemon_adaptor
)

add_executable(kpulse-daemon
    ${KPULSE_DAEMON_SOURCES}
    ${KPULSE_DAEMON_DBUS_ADAPTOR_SRCS}
)

target_include_directories(kpulse-daemon
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_BINARY_DIR}   # for generated kpulse_daemon_adaptor.h
        ${SYSTEMD_INCLUDE_DIRS}
)

target_link_libraries(kpulse-daemon
    PRIVATE
        kpulse
        Qt6::Core
        Qt6::DBus
        ${SYSTEMD_LIBRARIES}
)
