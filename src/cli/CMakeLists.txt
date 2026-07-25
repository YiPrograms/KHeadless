add_executable(kheadlessctl main.cpp)
target_link_libraries(kheadlessctl PRIVATE kheadlesscore Qt6::Core Qt6::DBus)
target_compile_definitions(kheadlessctl PRIVATE KHEADLESS_VERSION="${PROJECT_VERSION}")
install(TARGETS kheadlessctl RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
