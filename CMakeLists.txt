cmake_minimum_required(VERSION 3.16)
project(gdis VERSION 1.0 LANGUAGES C CXX)

set(CMAKE_BUILD_TYPE Release)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O2")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O2")

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(PkgConfig REQUIRED)
pkg_check_modules(GLIB2 REQUIRED glib-2.0)

# OpenGL/GLU: use Apple frameworks on macOS, pkg-config on Linux/other platforms
if(APPLE)
    find_library(OPENGL_LIB OpenGL)
    find_library(GLUT_LIB GLUT)
    if(NOT OPENGL_LIB OR NOT GLUT_LIB)
        message(FATAL_ERROR "OpenGL or GLUT framework not found. Install via MacPorts: sudo port install mesa-glu")
    endif()
    set(GL_LIBRARIES ${OPENGL_LIB} ${GLUT_LIB})
else()
    pkg_check_modules(GL REQUIRED gl glu)
    set(GL_LIBRARIES ${GL_LIBRARIES})
endif()

# GLib threading: ensure G_THREADS_ENABLED on macOS if not already defined by pkg-config
if(APPLE)
    # Check whether glib-2.0's cflags already include -DG_THREADS_ENABLED
    string(FIND "${GLIB2_CFLAGS}" "G_THREADS_ENABLED" _has_threads)
    if(_has_threads EQUAL -1)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DG_THREADS_ENABLED")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DG_THREADS_ENABLED")
    endif()
endif()

# Qt6 components
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets OpenGL Widgets Svg)
qt_standard_project_setup()
set(CMAKE_AUTOMOC ON)

# Optional: Grisu
option(USE_GRISU "Enable Grisu grid support" OFF)
if(USE_GRISU)
    find_package(kdsoap2-qt6 REQUIRED)
    add_definitions(-DUSE_GRISU)
endif()

# Optional: EPS support (cairo)
option(USE_CAIRO "Enable Cairo for EPS export" OFF)
if(USE_CAIRO)
    pkg_check_modules(CAIRO REQUIRED cairo)
    add_definitions(-DCAIRO_HAS_PS_SURFACE)
endif()

# Output directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../bin)

# Add subdirectories
add_subdirectory(src)
add_subdirectory(gui)

# Install data files
install(FILES bin/gdis.elements bin/gdis.library bin/gdis.manual
        DESTINATION ${CMAKE_INSTALL_PREFIX}/share/gdis)
