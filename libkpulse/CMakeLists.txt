qt6_add_dbus_interface(KPULSE_DBUS_INTERFACE_SRCS
    ${CMAKE_SOURCE_DIR}/daemon/src/dbus_interface.xml
    kpulse_dbus_interface
)

add_library(kpulse SHARED
    src/common.cpp
    src/event.cpp
    src/db.cpp
    src/ipc_client.cpp
    ${KPULSE_DBUS_INTERFACE_SRCS}
)

target_include_directories(kpulse
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(kpulse
    PUBLIC
        Qt6::Core
        Qt6::Sql
        Qt6::DBus
)
