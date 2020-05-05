cmake_minimum_required(VERSION 3.1.0 FATAL_ERROR)
# CMP0000: Call the cmake_minimum_required() command at the beginning of the top-level
# CMakeLists.txt file even before calling the project() command.
# The cmake_minimum_required(VERSION) command implicitly invokes the cmake_policy(VERSION)
# command to specify that the current project code is written for the given range of CMake
# versions.
project(lxqt-panel)

option(UPDATE_TRANSLATIONS "Update source translation translations/*.ts files" OFF)
option(WITH_SCREENSAVER_FALLBACK "Include support for converting the deprecated 'screensaver' plugin to 'quicklaunch'. This requires the lxqt-leave (lxqt-session) to be installed in runtime." ON)
# plugin-mainmenu
option(USE_MENU_CACHE "Use menu-cached (no noticable penalty even on a 2004 single core pentium if not used)" OFF)


# additional cmake files
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

macro(setByDefault VAR_NAME VAR_VALUE)
  if(NOT DEFINED ${VAR_NAME})
    set (${VAR_NAME} ${VAR_VALUE})
  endif(NOT DEFINED ${VAR_NAME})
endmacro()

include(GNUInstallDirs)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set(REQUIRED_QT_VERSION "5.7.1")
set(KF5_MINIMUM_VERSION "5.36.0")
set(LXQT_MINIMUM_VERSION "0.14.1")

find_package(Threads REQUIRED)
find_package(Qt5DBus ${REQUIRED_QT_VERSION} REQUIRED)
find_package(Qt5LinguistTools ${REQUIRED_QT_VERSION} REQUIRED)
find_package(Qt5Widgets ${REQUIRED_QT_VERSION} REQUIRED)
find_package(Qt5X11Extras ${REQUIRED_QT_VERSION} REQUIRED)
find_package(Qt5Xml ${REQUIRED_QT_VERSION} REQUIRED)
find_package(KF5WindowSystem ${KF5_MINIMUM_VERSION} REQUIRED)
find_package(X11 REQUIRED)
find_package(lxqt ${LXQT_MINIMUM_VERSION} REQUIRED)
find_package(lxqt-globalkeys ${LXQT_MINIMUM_VERSION} REQUIRED)
find_package(lxqt-globalkeys-ui ${LXQT_MINIMUM_VERSION} REQUIRED)

find_package(PkgConfig)
pkg_check_modules(XCB REQUIRED xcb xcb-util xcb-damage)
pkg_check_modules(XCOMPOSITE REQUIRED xcomposite)
pkg_check_modules(XDAMAGE REQUIRED xdamage)
pkg_check_modules(XRENDER REQUIRED xrender)

# Patch Version
set(LXQT_PANEL_PATCH_VERSION 1)
set(LXQT_PANEL_VERSION ${LXQT_MAJOR_VERSION}.${LXQT_MINOR_VERSION}.${LXQT_PANEL_PATCH_VERSION})
add_definitions("-DLXQT_PANEL_VERSION=\"${LXQT_PANEL_VERSION}\"")

include(LXQtPreventInSourceBuilds)
include(LXQtTranslate)

# All LXQtCompilerSettings except CMAKE_MODULE_LINKER_FLAGS work just fine
# So we reset only these Flags after loading LXQtCompilerSettings
# lxqt-build-tools:
#  set(CMAKE_MODULE_LINKER_FLAGS "-Wl,--no-undefined ${SYMBOLIC_FLAGS} ${CMAKE_MODULE_LINKER_FLAGS}")
message(STATUS "==OLD== CMAKE_MODULE_LINKER_FLAGS: ${CMAKE_MODULE_LINKER_FLAGS}")
set( OLD_CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS}")
include(LXQtCompilerSettings NO_POLICY_SCOPE)
set(CMAKE_MODULE_LINKER_FLAGS "${OLD_CMAKE_MODULE_LINKER_FLAGS} ${SYMBOLIC_FLAGS}")

# Warning: This must be before add_subdirectory(panel). Move with caution.
set(PLUGIN_DIR "${CMAKE_INSTALL_FULL_LIBDIR}/lxqt-panel")
add_definitions(
    -DPLUGIN_DIR=\"${PLUGIN_DIR}\"
)

remove_definitions("-DQT_NO_CAST_FROM_ASCII")
remove_definitions("-DQT_NO_CAST_FROM_BYTEARRAY")
remove_definitions("-DQT_NO_CAST_TO_ASCII")

message(STATUS "CMAKE Module linker flags: ${CMAKE_MODULE_LINKER_FLAGS}")
message(STATUS "Panel plugins location: ${PLUGIN_DIR}")

#########################################################################
# Plugin system
# You can enable/disable building of the plugin using cmake options.
#    cmake -DWORLDCLOCK_PLUGIN=Yes .. # Enable worldclock plugin
#    cmake -DWORLDCLOCK_PLUGIN=No ..  # Disable worldclock plugin

include("cmake/BuildPlugin.cmake")

set(ENABLED_PLUGINS) # list of enabled plugins
set(STATIC_PLUGINS) # list of statically linked plugins

#########################################################################

message(STATUS  "**************** The following plugins will be built ****************")
foreach (PLUGIN_STR ${ENABLED_PLUGINS})
    message(STATUS "  ${PLUGIN_STR}")
endforeach()
message(STATUS "*********************************************************************")

add_subdirectory(panel)
