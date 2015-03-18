project( MantidParaViewEventNexusReader )

add_paraview_plugin( MantidParaViewEventNexusReaderSMPlugin "1.0"
	SERVER_MANAGER_XML EventNexusReader.xml
	SERVER_MANAGER_SOURCES vtkEventNexusReader.cxx
)

# Add to the 'VatesParaViewPlugins' group in VS
set_property( TARGET MantidParaViewEventNexusReaderSMPlugin PROPERTY FOLDER "MantidVatesParaViewPlugins" )

include_directories ( SYSTEM ${QWT_INCLUDE_DIR} )

target_link_libraries( MantidParaViewEventNexusReaderSMPlugin LINK_PUBLIC ${MANTID_SUBPROJECT_LIBS} ${QWT_LIBRARIES})

# Put library into subfolder.
SET_TARGET_OUTPUT_DIRECTORY(MantidParaViewEventNexusReaderSMPlugin ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}/${PVPLUGINS_DIR}/${PVPLUGINS_SUBDIR})

install( TARGETS MantidParaViewEventNexusReaderSMPlugin ${SYSTEM_PACKAGE_TARGET} DESTINATION ${PVPLUGINS_DIR}/${PVPLUGINS_SUBDIR} )


