PROJECT(MantidParaViewMDEWSource)
include_directories( SYSTEM ${PARAVIEW_INCLUDE_DIRS} )
ADD_PARAVIEW_PLUGIN(MantidParaViewMDEWSourceSMPlugin "1.0"
	SERVER_MANAGER_XML MDEWSource.xml
	SERVER_MANAGER_SOURCES vtkMDEWSource.cxx)
set_target_properties ( MantidParaViewMDEWSourceSMPlugin PROPERTIES
  LIBRARY_OUTPUT_DIRECTORY ${PVPLUGIN_LIBRARY_OUTPUT_DIRECTORY}
  FOLDER "MantidVatesParaViewPlugins"
)

include_directories ( SYSTEM ${QWT5_INCLUDE_DIR} )
target_link_libraries( MantidParaViewMDEWSourceSMPlugin LINK_PRIVATE ${TCMALLOC_LIBRARIES_LINKTIME} 
${CORE_MANTIDLIBS}
DataObjects
VatesAPI
${POCO_LIBRARIES}
${Boost_LIBRARIES}
${vtkjsoncpp_LIBRARIES}
${QWT5_LIBRARIES}
Qt4::QtCore
)

if (OSX_VERSION VERSION_GREATER 10.8)
  set_target_properties ( MantidParaViewMDEWSourceSMPlugin PROPERTIES INSTALL_RPATH "@loader_path/../../Contents/Libraries;@loader_path/../../Contents/MacOS")
endif ()

install( TARGETS MantidParaViewMDEWSourceSMPlugin ${SYSTEM_PACKAGE_TARGET} DESTINATION ${PVPLUGINS_DIR}/${PVPLUGINS_SUBDIR} )
