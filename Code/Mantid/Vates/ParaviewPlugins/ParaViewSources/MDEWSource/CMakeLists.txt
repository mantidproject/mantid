PROJECT(MantidParaViewMDEWSource)

FIND_PACKAGE(ParaView REQUIRED)
INCLUDE(${PARAVIEW_USE_FILE})

ADD_PARAVIEW_PLUGIN(MantidParaViewMDEWSourceSMPlugin "1.0"
	SERVER_MANAGER_XML MDEWSource.xml
	SERVER_MANAGER_SOURCES vtkMDEWSource.cxx)

# Add to the 'VatesParaViewPlugins' group in VS
set_property( TARGET MantidParaViewMDEWSourceSMPlugin PROPERTY FOLDER "MantidVatesParaViewPlugins" )

target_link_libraries( MantidParaViewMDEWSourceSMPlugin 
${MANTID_SUBPROJECT_LIBS} )

# Put library into subfolder.
output_binary_to(MantidParaViewMDEWSourceSMPlugin ${PVPLUGINS_DIR})

install( TARGETS MantidParaViewMDEWSourceSMPlugin ${TARGET_TYPE} DESTINATION ${PVPLUGINS_DIR} )
