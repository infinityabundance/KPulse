add_executable(kpulse-ui
    src/main.cpp
    src/mainwindow.cpp
    src/event_model.cpp
    src/timeline_view.cpp
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
)
