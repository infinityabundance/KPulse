# libkpulse: shared library for KPulse core types, DB, and IPC

# Make absolutely sure automoc is on for this target
set(CMAKE_AUTOMOC ON)

# Generate DBus client interface from the daemon's XML description.
# This will create kpulse_dbus_interface.cpp / kpulse_dbus_interface.h
# in the binary directory for this target.
qt6_add_dbus_interface(KPULSE_DBUS_INTERFACE_SRCS
    ${CMAKE_SOURCE_DIR}/daemon/src/dbus_interface.xml
    kpulse_dbus_interface
)

add_library(kpulse SHARED
    src/common.cpp
    src/event.cpp
    src/db.cpp
    src/ipc_client.cpp
    include/kpulse/ipc_client.hpp
    ${KPULSE_DBUS_INTERFACE_SRCS}
)

# Public headers live in include/, generated DBus headers live in the
# build directory for this target (for kpulse_dbus_interface.h and moc_*).
target_include_directories(kpulse
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(kpulse
    PUBLIC
        Qt6::Core
        Qt6::Sql
        Qt6::DBus
)
