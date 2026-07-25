add_library(kheadlessuimodel STATIC dashboardcontroller.cpp)
target_link_libraries(kheadlessuimodel PUBLIC kheadlesscore Qt6::Core Qt6::DBus Qt6::Qml)
target_include_directories(kheadlessuimodel PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(kheadlessuimodel PUBLIC KHEADLESS_VERSION="${PROJECT_VERSION}")

qt_add_qml_module(kheadlessuimodel
    URI org.kde.kheadless
    VERSION 1.0
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/org/kde/kheadless
    RESOURCE_PREFIX /qt/qml
    QML_FILES
        qml/KHeadlessView.qml
        qml/OverviewPage.qml
        qml/DisplaysPage.qml
        qml/AccessPage.qml
        qml/DiagnosticsPage.qml
)

add_executable(kheadless-settings main.cpp)
target_link_libraries(kheadless-settings PRIVATE
    kheadlessuimodel
    kheadlessuimodelplugin
    Qt6::Gui
    Qt6::Quick
    Qt6::QuickControls2
)

qt_add_resources(kheadless-settings "settings"
    PREFIX "/org/kde/kheadless/settings"
    FILES qml/Main.qml
)

install(TARGETS kheadless-settings RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES org.kde.kheadless.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)

if(BUILD_TESTING)
    add_test(NAME test_ui_startup COMMAND kheadless-settings --check)
    set_tests_properties(test_ui_startup PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_QUICK_BACKEND=software"
        TIMEOUT 20
    )
endif()
