add_subdirectory(core)
add_subdirectory(daemon)
add_subdirectory(cli)

if(KHEADLESS_BUILD_UI)
    add_subdirectory(ui)
endif()

if(KHEADLESS_BUILD_KCM)
    add_subdirectory(kcm)
endif()

