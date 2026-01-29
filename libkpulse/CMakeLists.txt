add_library(kpulse SHARED
    src/common.cpp
    src/event.cpp
    src/db.cpp
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
