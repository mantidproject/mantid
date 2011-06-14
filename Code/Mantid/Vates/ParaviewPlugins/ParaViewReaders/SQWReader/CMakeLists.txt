project( MantidParaViewSQWReader )

add_paraview_plugin( MantidParaViewSQWReaderSMPlugin "1.0"
	SERVER_MANAGER_XML SQWReader.xml
	SERVER_MANAGER_SOURCES vtkSQWReader.cxx
    GUI_RESOURCE_FILES SQWReaderGUI.xml
)

target_link_libraries( MantidParaViewSQWReaderSMPlugin 
${MANTID_SUBPROJECT_LIBS} )

# Put library into subfolder.
#output_binary_to( MantidParaViewSQWReaderSMPlugin PVPlugins)

# Windows-only at the moment.
if( ${CMAKE_SYSTEM_NAME} STREQUAL "Windows" )
  install(TARGETS MantidParaViewSQWReaderSMPlugin RUNTIME DESTINATION ${LIB_DIR}/PVPlugins)
endif( ${CMAKE_SYSTEM_NAME} STREQUAL "Windows" )


