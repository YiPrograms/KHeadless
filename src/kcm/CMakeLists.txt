kcmutils_add_qml_kcm(kcm_kheadless
    SOURCES kcm.cpp
)

target_link_libraries(kcm_kheadless PRIVATE
    kheadlessuimodel
    KF6::KCMUtilsQuick
    KF6::CoreAddons
    Qt6::Quick
)
