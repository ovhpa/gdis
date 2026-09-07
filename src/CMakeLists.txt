# GDIS C Core Library
# All non-GUI source files compiled as C

set(CORE_SOURCES
    main.c coords.c connect.c matrix.c module.c task.c model.c
    type.c measure.c parse.c edit.c elem.c select.c surface.c
    analysis.c spatial.c render.c numeric.c project.c
    molsurf.c hirshfeld.c zone.c contents.c quaternion.c zmatrix.c
    library.c geometry.c space.c sginfo.c sgclib.c sgio.c error.c
    scan.c camera.c defect.c colourlib.c crystal_graph.c mdi.c
    mesch.c mesch_core.c host.c job.c command.c undo.c count.c
    ff.c ff_gulp.c test.c
    file.c file_cif.c file_gulp.c file_gmf.c file_marvin.c file_xtl.c
    file_arc.c file_xyz.c file_fdf.c file_vasp.c file_gms.c file_diff.c file_xml.c
    file_abinit.c file_pdb.c file_povray.c file_nwchem.c file_castep.c
    file_gauss.c file_qe.c file_rietica.c file_geomview.c file_cssr.c file_cel.c
    file_dmol.c file_dlpoly.c file_bgf.c file_cgf.c file_dlp.c
    file_gromacs.c file_meta.c file_mol2.c file_uspex.c
    dock.c image.c
    gl_main.c gl_primitives.c gl_stereo.c gl_graph.c gl_varray.c
    plots.c symmetry.c
    gui_still_valid.c
    gl_text.cpp
)

if(USE_GRISU)
    list(APPEND CORE_SOURCES
        grisu_client.c soapC.c soapClient.c logging.c
    )
endif()

add_library(gdis_core OBJECT ${CORE_SOURCES})

target_include_directories(gdis_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${GLIB2_INCLUDE_DIRS}
)

# Define WITH_GUI to enable GUI code paths
target_compile_definitions(gdis_core PUBLIC WITH_GUI=1)
# Enable glib threading support for task queue (already enabled)
# target_compile_definitions(gdis_core PUBLIC G_THREADS_ENABLED)

target_link_libraries(gdis_core PUBLIC
    ${GLIB2_LIBRARIES}
    ${GL_LIBRARIES}
    Qt6::Core Qt6::Gui
    m dl
)

# Cairo for EPS export (optional)
if(USE_CAIRO)
    target_include_directories(gdis_core PRIVATE ${CAIRO_INCLUDE_DIRS})
    target_link_libraries(gdis_core PRIVATE ${CAIRO_LIBRARIES})
endif()
