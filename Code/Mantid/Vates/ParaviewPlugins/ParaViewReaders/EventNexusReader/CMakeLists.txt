project( MantidParaViewEventNexusReader )
include_directories( SYSTEM ${PARAVIEW_INCLUDE_DIRS} )
add_paraview_plugin( MantidParaViewEventNexusReaderSMPlugin "1.0"
	SERVER_MANAGER_XML EventNexusReader.xml
	SERVER_MANAGER_SOURCES vtkEventNexusReader.cxx
	GUI_RESOURCE_FILES EventNexusReaderGUI.xml
)

# Add to the 'VatesParaViewPlugins' group in VS
set_property( TARGET MantidParaViewEventNexusReaderSMPlugin PROPERTY FOLDER "MantidVatesParaViewPlugins" )

include_directories ( SYSTEM ${QWT_INCLUDE_DIR} )

target_link_libraries( MantidParaViewEventNexusReaderSMPlugin 
${MANTID_SUBPROJECT_LIBS} ${QWT_LIBRARIES})

# Put library into subfolder.
SET_TARGET_OUTPUT_DIRECTORY(MantidParaViewEventNexusReaderSMPlugin ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}/${PVPLUGINS_DIR}/${PVPLUGINS_SUBDIR})

install( TARGETS MantidParaViewEventNexusReaderSMPlugin ${SYSTEM_PACKAGE_TARGET} DESTINATION ${PVPLUGINS_DIR}/${PVPLUGINS_SUBDIR} )


