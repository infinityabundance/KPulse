add_executable(kpulse-ui
    src/main.cpp
)

target_link_libraries(kpulse-ui
    PRIVATE
        kpulse
        Qt6::Core
        Qt6::Widgets
)
