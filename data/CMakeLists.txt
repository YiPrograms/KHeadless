configure_file(systemd/kheadlessd.service.in kheadlessd.service @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/kheadlessd.service
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/systemd/user
)
install(FILES systemd/kheadless-session.target
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/systemd/user
)
install(FILES org.kde.kheadlessd.desktop
    DESTINATION ${CMAKE_INSTALL_DATADIR}/applications
)
