configure_file(systemd/app-org.kde.kheadlessd.service.in app-org.kde.kheadlessd.service @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/app-org.kde.kheadlessd.service
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/systemd/user
)
install(FILES systemd/kheadless-session.target
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/systemd/user
)
configure_file(org.kde.kheadlessd.desktop.in org.kde.kheadlessd.desktop @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/org.kde.kheadlessd.desktop
    DESTINATION ${CMAKE_INSTALL_DATADIR}/applications
)
