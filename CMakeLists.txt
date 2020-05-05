cmake_minimum_required(VERSION 3.1.0 FATAL_ERROR)
project(lxqt-panel)

find_package(Threads REQUIRED)
find_package(Qt5Widgets REQUIRED)
find_package(Qt5X11Extras REQUIRED)
find_package(KF5WindowSystem REQUIRED)
find_package(X11 REQUIRED)
find_package(lxqt REQUIRED)

find_package(PkgConfig)
pkg_check_modules(XCB REQUIRED xcb xcb-util xcb-damage)
pkg_check_modules(XCOMPOSITE REQUIRED xcomposite)
pkg_check_modules(XDAMAGE REQUIRED xdamage)
pkg_check_modules(XRENDER REQUIRED xrender)

remove_definitions("-DQT_NO_CAST_FROM_ASCII")
remove_definitions("-DQT_NO_CAST_FROM_BYTEARRAY")
remove_definitions("-DQT_NO_CAST_TO_ASCII")

add_subdirectory(panel)
