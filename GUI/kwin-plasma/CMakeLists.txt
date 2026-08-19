cmake_minimum_required(VERSION 3.16)

set(QT_MIN_VERSION "6.5.0")
set(KF6_MIN_VERSION "6.0.0")
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

project(kcm_kwinweb)

find_package(ECM ${KF6_MIN_VERSION} REQUIRED NO_MODULE)



include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)

include(GNUInstallDirs)
kcmutils_add_qml_kcm(kcm_kwinweb)

target_sources(kcm_kwinweb PRIVATE
    main_settings.cpp
    main_settings.hpp
    SettingsBackend.cpp
    SettingsBackend.hpp
    $<TARGET_OBJECTS:webkde_core>
)




target_include_directories(kcm_kwinweb PRIVATE
     ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/config
    ${CMAKE_SOURCE_DIR}/src/header
    ${CMAKE_SOURCE_DIR}/src/lib
    /usr/include/kwin
    /usr/include/kwin/core
)



target_link_libraries(

    kcm_kwinweb

    PRIVATE


    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::DBus


    KF6::CoreAddons
    KF6::ConfigCore
    KF6::ConfigWidgets
    KF6::KCMUtils
    KF6::I18n
    KF6::AuthCore
)
install(
    FILES kcm_kwinweb.json
    DESTINATION ${CMAKE_INSTALL_DATADIR}/kservices6
)
#
# KAuth helper
#
find_package(KF6Auth REQUIRED)

add_executable(
    kwinweb_helper
    khelper.cpp
    $<TARGET_OBJECTS:webkde_core>
)

target_link_libraries(
    kwinweb_helper
    PRIVATE
    KF6::AuthCore
    KF6::CoreAddons
    Qt6::Core
    webkde_core

)
install(
    TARGETS kwinweb_helper
    DESTINATION ${KAUTH_HELPER_INSTALL_DIR}
)
kauth_install_helper_files(
    kwinweb_helper
    org.kwinweb
    root
)
## Ensure the polkit policy file is available in the build tree for helper installation
configure_file(org.kwinweb.policy
    ${CMAKE_CURRENT_BINARY_DIR}/org.kwinweb.policy COPYONLY)
kauth_install_actions(
    org.kwinweb
    org.kwinweb.actions
)

install(
    FILES assets/sakichansakichansakichansakichansakichansakichansakichansakichan.svg
    DESTINATION ${KDE_INSTALL_ICONDIR}/hicolor/scalable/apps
    RENAME org.kwinweb.svg
)

install(
    FILES org.kwinweb.desktop
    DESTINATION ${KDE_INSTALL_APPDIR}
)
