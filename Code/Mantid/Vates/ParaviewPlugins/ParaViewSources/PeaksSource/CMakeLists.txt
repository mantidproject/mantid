PROJECT(MantidParaViewPeaksSource)
include_directories( SYSTEM ${PARAVIEW_INCLUDE_DIRS} )
ADD_PARAVIEW_PLUGIN(MantidParaViewPeaksSourceSMPlugin "1.0"
	SERVER_MANAGER_XML PeaksSource.xml
	SERVER_MANAGER_SOURCES vtkPeaksSource.cxx)

# Add to the 'VatesParaViewPlugins' group in VS
set_property( TARGET MantidParaViewPeaksSourceSMPlugin PROPERTY FOLDER "MantidVatesParaViewPlugins" )

target_link_libraries( MantidParaViewPeaksSourceSMPlugin LINK_PUBLIC
${MANTID_SUBPROJECT_LIBS} )

if (OSX_VERSION VERSION_GREATER 10.8)
  set_target_properties ( MantidParaViewPeaksSourceSMPlugin PROPERTIES INSTALL_RPATH "@loader_path/../../Contents/Libraries;@loader_path/../../Contents/MacOS")
endif ()

# Put library into subfolder.
SET_TARGET_OUTPUT_DIRECTORY(MantidParaViewPeaksSourceSMPlugin ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}/${PVPLUGINS_DIR}/${PVPLUGINS_SUBDIR})

install( TARGETS MantidParaViewPeaksSourceSMPlugin ${SYSTEM_PACKAGE_TARGET} DESTINATION ${PVPLUGINS_DIR}/${PVPLUGINS_SUBDIR} )
