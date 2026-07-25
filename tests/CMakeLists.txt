function(kheadless_add_test name)
    qt_add_executable(${name} ${ARGN})
    target_link_libraries(${name} PRIVATE kheadlesscore Qt6::Test)
    add_test(NAME ${name} COMMAND ${name})
endfunction()

kheadless_add_test(test_monitorlayout test_monitorlayout.cpp)
kheadless_add_test(test_layoutcontroller test_layoutcontroller.cpp)
kheadless_add_test(test_connectionregistry test_connectionregistry.cpp)
kheadless_add_test(test_storage test_storage.cpp)

