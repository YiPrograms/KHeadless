function(assert_file_contains path expected)
    file(READ "${path}" contents)
    string(FIND "${contents}" "${expected}" match_index)
    if(match_index EQUAL -1)
        message(FATAL_ERROR "${path} does not contain: ${expected}")
    endif()
endfunction()

assert_file_contains(
    "${DESKTOP_FILE}"
    "Exec=${BINDIR}/kheadlessd"
)
assert_file_contains(
    "${DESKTOP_FILE}"
    "X-KDE-Wayland-Interfaces=org_kde_kwin_fake_input,zkde_screencast_unstable_v1"
)
assert_file_contains(
    "${SYSTEMD_FILE}"
    "BusName=org.kde.KHeadless1"
)
assert_file_contains(
    "${SYSTEMD_FILE}"
    "RuntimeDirectory=krdp"
)
assert_file_contains(
    "${SYSTEMD_FILE}"
    "ReadWritePaths=%t/krdp"
)
assert_file_contains(
    "${DBUS_FILE}"
    "SystemdService=app-org.kde.kheadlessd.service"
)
