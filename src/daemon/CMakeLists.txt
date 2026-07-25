add_executable(kheadlessd
    main.cpp
    rdpbackend.cpp
    service.cpp
)

target_link_libraries(kheadlessd PRIVATE
    kheadlesscore
    Qt6::Core
    Qt6::DBus
    Qt6::Network
    Qt6Keychain::Qt6Keychain
)

if(KHEADLESS_BUILD_KRDP)
    target_sources(kheadlessd PRIVATE
        embeddedrdpbackend.cpp
        rdplayoutconversion.cpp
    )
    target_compile_definitions(kheadlessd PRIVATE KHEADLESS_HAVE_KRDP=1)
    target_link_libraries(kheadlessd PRIVATE KRdp::KRdp Qt6::Gui KF6::GuiAddons)
    if(BUILD_TESTING)
        if(TARGET displaycontroltest)
            add_dependencies(kheadlessd displaycontroltest)
        endif()
        if(TARGET clipboardtest)
            add_dependencies(kheadlessd clipboardtest)
        endif()
    endif()
endif()

install(TARGETS kheadlessd RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
configure_file(org.kde.KHeadless1.service org.kde.KHeadless1.service @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/org.kde.KHeadless1.service
    DESTINATION ${CMAKE_INSTALL_DATADIR}/dbus-1/services
)
