add_library(kheadlesscore STATIC
    connectionregistry.cpp
    credentialstore.cpp
    dbustypes.h
    kscreenbackend.cpp
    layoutcontroller.cpp
    monitorlayout.cpp
    outputbackend.h
    settingsstore.cpp
)

target_include_directories(kheadlesscore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(kheadlesscore
    PUBLIC
        Qt6::Core
        Qt6::DBus
    PRIVATE
        KF6::Screen
        PkgConfig::SODIUM
)
target_compile_definitions(kheadlesscore PUBLIC KHEADLESS_VERSION="${PROJECT_VERSION}")
