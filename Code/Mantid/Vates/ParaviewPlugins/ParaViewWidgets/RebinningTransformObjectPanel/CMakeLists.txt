# Include Qt widgets
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/../QtWidgets )

# So that source file shows in VS.
set( INCLUDE_FILES RebinningTransformObjectPanel.h )
set( SRC_FILES RebinningTransformObjectPanel.cxx )

qt4_wrap_cpp( MOC_SRCS RebinningTransformObjectPanel.h )
add_paraview_object_panel( IFACES IFACE_SRCS 
                           CLASS_NAME RebinningTransformObjectPanel
                           XML_NAME RebinningTransformOperator XML_GROUP filters)
# Deal with Intel compiler warnings in generated files
if ( ${CMAKE_CXX_COMPILER_ID} MATCHES "Intel" )
  set_source_files_properties ( ${IFACES} ${IFACE_SRCS} PROPERTIES COMPILE_FLAGS -wd1170 )
endif ()
add_paraview_plugin( MantidParaViewRebinningTransformObjectPanel "1.0"
                     GUI_INTERFACES ${IFACES}
                     SOURCES ${MOC_SRCS} ${IFACE_SRCS} ${INCLUDE_FILES} 
                     ${SRC_FILES} )
# Add to the 'VatesParaViewPlugins' group in VS
set_property( TARGET MantidParaViewRebinningTransformObjectPanel PROPERTY FOLDER "MantidVatesParaViewPlugins" )
target_link_libraries( MantidParaViewRebinningTransformObjectPanel 
MantidParaViewQtWidgets )

# Put library into subfolder.
SET_TARGET_OUTPUT_DIRECTORY( MantidParaViewRebinningTransformObjectPanel  ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}/${PVPLUGINS_DIR}/${PVPLUGINS_SUBDIR})

install( TARGETS MantidParaViewRebinningTransformObjectPanel ${SYSTEM_PACKAGE_TARGET} DESTINATION ${PVPLUGINS_DIR}/${PVPLUGINS_SUBDIR} )


