project( MantidParaViewNexusPeaksReader )
include_directories( SYSTEM ${PARAVIEW_INCLUDE_DIRS} )
add_paraview_plugin( MantidParaViewNexusPeaksReaderSMPlugin "1.0"
	SERVER_MANAGER_XML NexusPeaksReader.xml
	SERVER_MANAGER_SOURCES vtkNexusPeaksReader.cxx
)
set_target_properties ( MantidParaViewNexusPeaksReaderSMPlugin PROPERTIES
  LIBRARY_OUTPUT_DIRECTORY ${PVPLUGIN_LIBRARY_OUTPUT_DIRECTORY}
  FOLDER "MantidVatesParaViewPlugins"
)

target_link_libraries( MantidParaViewNexusPeaksReaderSMPlugin LINK_PRIVATE ${TCMALLOC_LIBRARIES_LINKTIME} 
${CORE_MANTIDLIBS}
DataObjects
VatesAPI
${POCO_LIBRARIES}
${Boost_LIBRARIES}
${NEXUS_LIBRARIES}
${NEXUS_C_LIBRARIES})

if (OSX_VERSION VERSION_GREATER 10.8)
  set_target_properties ( MantidParaViewNexusPeaksReaderSMPlugin PROPERTIES INSTALL_RPATH "@loader_path/../../Contents/Libraries;@loader_path/../../Contents/MacOS")
endif ()

install( TARGETS MantidParaViewNexusPeaksReaderSMPlugin ${SYSTEM_PACKAGE_TARGET} DESTINATION ${PVPLUGINS_DIR}/${PVPLUGINS_SUBDIR} )


