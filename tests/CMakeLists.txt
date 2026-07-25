function(kheadless_add_test name)
    qt_add_executable(${name} ${ARGN})
    target_link_libraries(${name} PRIVATE kheadlesscore Qt6::Test)
    add_test(NAME ${name} COMMAND ${name})
endfunction()

kheadless_add_test(test_monitorlayout test_monitorlayout.cpp)
kheadless_add_test(test_layoutcontroller test_layoutcontroller.cpp)
kheadless_add_test(test_connectionregistry test_connectionregistry.cpp)
kheadless_add_test(test_storage test_storage.cpp)

if(KHEADLESS_BUILD_KRDP)
    qt_add_executable(test_rdplayout
        test_rdplayout.cpp
        ${CMAKE_SOURCE_DIR}/src/daemon/rdplayoutconversion.cpp
    )
    target_include_directories(test_rdplayout PRIVATE
        ${CMAKE_SOURCE_DIR}/src/daemon
    )
    target_link_libraries(test_rdplayout PRIVATE
        kheadlesscore
        KRdp::KRdp
        Qt6::Test
    )
    add_test(NAME test_rdplayout COMMAND test_rdplayout)
endif()
