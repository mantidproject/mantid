project( MantidParaViewSQWEventReader )

add_paraview_plugin( MantidParaViewSQWEventReaderSMPlugin "1.0"
	SERVER_MANAGER_XML SQWEventReader.xml
	SERVER_MANAGER_SOURCES vtkSQWEventReader.cxx
)

# Add to the 'VatesParaViewPlugins' group in VS
set_property( TARGET MantidParaViewSQWEventReaderSMPlugin PROPERTY FOLDER "MantidVatesParaViewPlugins" )

include_directories ( SYSTEM ${QWT_INCLUDE_DIR} )

target_link_libraries( MantidParaViewSQWEventReaderSMPlugin LINK_PUBLIC ${MANTID_SUBPROJECT_LIBS} ${QWT_LIBRARIES})

# Put library into subfolder.
SET_TARGET_OUTPUT_DIRECTORY(MantidParaViewSQWEventReaderSMPlugin ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}/${PVPLUGINS_DIR}/${PVPLUGINS_SUBDIR})

install( TARGETS MantidParaViewSQWEventReaderSMPlugin ${SYSTEM_PACKAGE_TARGET} DESTINATION ${PVPLUGINS_DIR}/${PVPLUGINS_SUBDIR} )


