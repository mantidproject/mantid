PROJECT(MantidParaViewMDEWSource)
include_directories( SYSTEM ${PARAVIEW_INCLUDE_DIRS} )
ADD_PARAVIEW_PLUGIN(MantidParaViewMDEWSourceSMPlugin "1.0"
	SERVER_MANAGER_XML MDEWSource.xml
	SERVER_MANAGER_SOURCES vtkMDEWSource.cxx)

# Add to the 'VatesParaViewPlugins' group in VS
set_property( TARGET MantidParaViewMDEWSourceSMPlugin PROPERTY FOLDER "MantidVatesParaViewPlugins" )
include_directories ( SYSTEM ${QWT_INCLUDE_DIR} )
target_link_libraries( MantidParaViewMDEWSourceSMPlugin 
${MANTID_SUBPROJECT_LIBS} ${QWT_LIBRARIES})

# Put library into subfolder.
SET_TARGET_OUTPUT_DIRECTORY(MantidParaViewMDEWSourceSMPlugin ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}/${PVPLUGINS_DIR}/${PVPLUGINS_SUBDIR})

install( TARGETS MantidParaViewMDEWSourceSMPlugin ${SYSTEM_PACKAGE_TARGET} DESTINATION ${PVPLUGINS_DIR}/${PVPLUGINS_SUBDIR} )
