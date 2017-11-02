PROJECT(MantidParaViewSinglePeakMarkerSource)
include_directories( SYSTEM ${PARAVIEW_INCLUDE_DIRS} )
ADD_PARAVIEW_PLUGIN(MantidParaViewSinglePeakMarkerSourceSMPlugin "1.0"
	SERVER_MANAGER_XML SinglePeakMarkerSource.xml
	SERVER_MANAGER_SOURCES vtkSinglePeakMarkerSource.cxx)
set_target_properties ( MantidParaViewSinglePeakMarkerSourceSMPlugin PROPERTIES
  LIBRARY_OUTPUT_DIRECTORY ${PVPLUGIN_LIBRARY_OUTPUT_DIRECTORY}
  FOLDER "MantidVatesParaViewPlugins"
)

target_link_libraries( MantidParaViewSinglePeakMarkerSourceSMPlugin LINK_PRIVATE ${TCMALLOC_LIBRARIES_LINKTIME} ${CORE_MANTIDLIBS} VatesAPI )

if (OSX_VERSION VERSION_GREATER 10.8)
  set_target_properties ( MantidParaViewSinglePeakMarkerSourceSMPlugin PROPERTIES INSTALL_RPATH "@loader_path/../../Contents/Libraries;@loader_path/../../Contents/MacOS")
endif ()

install( TARGETS MantidParaViewSinglePeakMarkerSourceSMPlugin  ${SYSTEM_PACKAGE_TARGET} DESTINATION ${PVPLUGINS_DIR}/${PVPLUGINS_SUBDIR} )
