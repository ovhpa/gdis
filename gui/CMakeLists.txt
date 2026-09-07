# GDIS Qt6 GUI
# C++ GUI layer using Qt6

set(GUI_SOURCES
    main.cpp
    mainwindow.cpp
    glcanvas.cpp
    renderdialog.cpp
    animatedialog.cpp
    editdialog.cpp
    isosurfacesdialog.cpp
    zmatrixdialog.cpp
    periodictabledialog.cpp
    elementeditdialog.cpp
    measurementdialog.cpp
    diffractiondialog.cpp
    mdanalysisdialog.cpp
    mdidialog.cpp
    dockingdialog.cpp
    dislocationdialog.cpp
    plotsdialog.cpp
    graphcontrolsdialog.cpp
    gulpdialog.cpp
    gamessdialog.cpp
    montydialog.cpp
    siestadialog.cpp
    vaspdialog.cpp
    uspexdialog.cpp
    taskmanagerdialog.cpp
    execpathsdialog.cpp
    textoverlay.cpp
    svg_utils.cpp
    qt_api.c
    surfacedialog.cpp
)

add_executable(gdis ${GUI_SOURCES})
set_target_properties(gdis PROPERTIES AUTOMOC ON)

# Link against the C core object files
# Core objects must come AFTER GUI objects for proper symbol resolution
# Use LINK_LIBRARIES to append them at the end of the link command
target_link_libraries(gdis PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::OpenGL
    Qt6::OpenGLWidgets
    Qt6::Svg
    ${GLIB2_LIBRARIES}
    ${GL_LIBRARIES}
    m dl
)

# Manually add OpenGLWidgets if CMake config did not export it.
# Note: Qt 6 bundles OpenGLWidgets in libqt6opengl6 (no separate -dev package).
if(NOT TARGET Qt6::OpenGLWidgets)
    find_library(QT_OPENGL_WIDGETS_LIB Qt6OpenGLWidgets
        PATHS "$ENV{QTDIR}/lib" "${CMAKE_PREFIX_PATH}/lib"
              "/opt/local/lib" "/usr/local/lib"
              "/usr/lib" "/usr/lib/x86_64-linux-gnu")
    if(QT_OPENGL_WIDGETS_LIB)
        # Find Qt include prefix by checking common locations
        set(_qt_inc_prefix "")
        foreach(_candidate
                "$ENV{QTDIR}/include"
                "${CMAKE_PREFIX_PATH}/include"
                "/opt/local/include"
                "/usr/local/include"
                "/usr/include/x86_64-linux-gnu/qt6"
                "/usr/include/qt6")
            if(EXISTS "${_candidate}/QtOpenGLWidgets/qopenglwidget.h")
                set(_qt_inc_prefix "${_candidate}")
                break()
            endif()
        endforeach()
        if(NOT _qt_inc_prefix)
            message(FATAL_ERROR
                "Qt6::OpenGLWidgets headers not found. Ensure Qt6 is properly installed.")
        endif()
        add_library(Qt6::OpenGLWidgets STATIC IMPORTED)
        set_target_properties(Qt6::OpenGLWidgets PROPERTIES
            IMPORTED_LOCATION "${QT_OPENGL_WIDGETS_LIB}"
            INTERFACE_INCLUDE_DIRECTORIES "${_qt_inc_prefix}/QtOpenGLWidgets"
            INTERFACE_LINK_LIBRARIES "Qt6::OpenGL;Qt6::Widgets")
    else()
        message(FATAL_ERROR
            "Qt6::OpenGLWidgets library not found. Ensure Qt6 is properly installed.\n"
            "  Ubuntu/Debian: apt install libqt6opengl6-dev\n"
            "  MacPorts:      port install qt6-base")
    endif()
endif()

# Include directories for the C core headers
target_include_directories(gdis PRIVATE ${CMAKE_SOURCE_DIR}/src)

# Set output to bin/ directory
set_target_properties(gdis PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../bin
)

# Append core object files after GUI objects for proper link order
# This ensures C++ GUI symbols can resolve C core symbols
# Use OBJECT_DEPENDENCIES to ensure core objects are built, then link them
target_link_libraries(gdis PRIVATE $<TARGET_OBJECTS:gdis_core>)

# Ensure core include dirs are available for the OBJECT library expansion
target_include_directories(gdis PRIVATE $<TARGET_PROPERTY:gdis_core,INCLUDE_DIRECTORIES>)
