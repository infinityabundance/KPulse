# KPulse launcher: starts daemon + tray + UI

add_executable(kpulse
    src/main.cpp
)

target_link_libraries(kpulse
    Qt6::Core
    Qt6::DBus
)

target_compile_features(kpulse PRIVATE cxx_std_20)

install(TARGETS kpulse RUNTIME DESTINATION bin)
