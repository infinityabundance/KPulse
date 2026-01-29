add_executable(kpulse-tray
    src/main.cpp
)

target_link_libraries(kpulse-tray
    Qt6::Widgets
    Qt6::DBus
)

target_compile_features(kpulse-tray PRIVATE cxx_std_20)

install(TARGETS kpulse-tray RUNTIME DESTINATION bin)
