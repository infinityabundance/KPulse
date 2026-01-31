# KPulse main UI

add_executable(kpulse-ui
    src/main.cpp
    src/mainwindow.cpp
    src/event_model.cpp
    src/timeline_view.cpp
    ${CMAKE_SOURCE_DIR}/assets/kpulse.qrc
)

target_include_directories(kpulse-ui
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(kpulse-ui
    PRIVATE
        kpulse
        Qt6::Core
        Qt6::Widgets
        Qt6::DBus
        Qt6::Sql
        Qt6::Svg
)
