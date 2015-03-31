project( MantidParaViewNexusPeaksReader )
include_directories( SYSTEM ${PARAVIEW_INCLUDE_DIRS} )
add_paraview_plugin( MantidParaViewNexusPeaksReaderSMPlugin "1.0"
	SERVER_MANAGER_XML NexusPeaksReader.xml
	SERVER_MANAGER_SOURCES vtkNexusPeaksReader.cxx
)

# Add to the 'VatesParaViewPlugins' group in VS
set_property( TARGET MantidParaViewNexusPeaksReaderSMPlugin PROPERTY FOLDER "MantidVatesParaViewPlugins" )

target_link_libraries( MantidParaViewNexusPeaksReaderSMPlugin LINK_PUBLIC
${MANTID_SUBPROJECT_LIBS} ${NEXUS_C_LIBRARIES})

# Put library into subfolder.
SET_TARGET_OUTPUT_DIRECTORY(MantidParaViewNexusPeaksReaderSMPlugin ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}/${PVPLUGINS_DIR}/${PVPLUGINS_SUBDIR})

install( TARGETS MantidParaViewNexusPeaksReaderSMPlugin ${SYSTEM_PACKAGE_TARGET} DESTINATION ${PVPLUGINS_DIR}/${PVPLUGINS_SUBDIR} )


