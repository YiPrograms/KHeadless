cmake_minimum_required(VERSION 3.24)

project(KHeadless VERSION 0.1.0 LANGUAGES CXX)

include(CTest)
include(GNUInstallDirs)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

option(KHEADLESS_BUILD_UI "Build the standalone Qt Quick settings application" ON)
option(KHEADLESS_BUILD_KCM "Build the Plasma System Settings module" ON)
option(KHEADLESS_BUILD_KRDP "Build the embedded KRdp backend from the submodule" ON)
option(BUILD_SHARED_LIBS "Build plugin libraries dynamically" ON)

find_package(Qt6 6.7 REQUIRED COMPONENTS Core DBus Network)
qt_standard_project_setup(REQUIRES 6.7)
find_package(KF6Screen 6.3 REQUIRED)
find_package(Qt6Keychain CONFIG REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(SODIUM REQUIRED IMPORTED_TARGET libsodium)

if(KHEADLESS_BUILD_UI)
    find_package(Qt6 6.7 REQUIRED COMPONENTS Gui Qml Quick QuickControls2)
    if(QT_KNOWN_POLICY_QTP0004)
        qt_policy(SET QTP0004 NEW)
    endif()
endif()

if(KHEADLESS_BUILD_KCM)
    find_package(ECM 6.10 QUIET NO_MODULE)
    if(NOT ECM_FOUND)
        message(WARNING "Extra CMake Modules were not found; disabling KCM")
        set(KHEADLESS_BUILD_KCM OFF)
    else()
        list(APPEND CMAKE_MODULE_PATH ${ECM_MODULE_PATH})
        include(KDEInstallDirs)
        set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
        find_package(KF6KCMUtils 6.10 QUIET)
        find_package(KF6CoreAddons 6.10 QUIET)
        if(NOT KF6KCMUtils_FOUND OR NOT KF6CoreAddons_FOUND)
            message(WARNING "KF6 KCM development files were not found; disabling KCM")
            set(KHEADLESS_BUILD_KCM OFF)
        endif()
    endif()
endif()

if(KHEADLESS_BUILD_KRDP)
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/krdp/CMakeLists.txt")
        message(FATAL_ERROR "Initialize the KRdp submodule before enabling KHEADLESS_BUILD_KRDP")
    endif()
    find_package(Qt6 6.7 REQUIRED COMPONENTS Gui WaylandClient)
    if(Qt6Gui_VERSION VERSION_GREATER_EQUAL "6.10.0")
        find_package(Qt6GuiPrivate ${Qt6Gui_VERSION} REQUIRED NO_MODULE)
    endif()
    find_package(KF6GuiAddons 6.10 REQUIRED)
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(BUILD_PLASMA_SESSION ON CACHE BOOL "" FORCE)
    add_subdirectory(third_party/krdp EXCLUDE_FROM_ALL)
    install(TARGETS KRdp
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
endif()

add_subdirectory(src)
add_subdirectory(data)

if(BUILD_TESTING)
    find_package(Qt6 6.7 REQUIRED COMPONENTS Test)
    add_subdirectory(tests)
endif()

install(FILES README.md LICENSE DESTINATION ${CMAKE_INSTALL_DOCDIR})
install(DIRECTORY docs/ DESTINATION ${CMAKE_INSTALL_DOCDIR})
install(PROGRAMS tools/check-dependencies
    DESTINATION ${CMAKE_INSTALL_LIBEXECDIR}/kheadless
)
