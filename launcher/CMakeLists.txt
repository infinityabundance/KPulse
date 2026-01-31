# KPulse launcher: starts daemon + tray + UI

add_executable(kpulse-launcher
    src/main.cpp
)

target_link_libraries(kpulse-launcher
    Qt6::Core
    Qt6::DBus
)

target_compile_features(kpulse-launcher PRIVATE cxx_std_20)

set_target_properties(kpulse-launcher PROPERTIES OUTPUT_NAME kpulse)

install(TARGETS kpulse-launcher RUNTIME DESTINATION bin)
