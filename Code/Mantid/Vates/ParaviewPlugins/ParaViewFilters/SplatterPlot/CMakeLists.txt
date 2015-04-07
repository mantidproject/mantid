PROJECT(SplatterPlot)
include_directories( SYSTEM ${PARAVIEW_INCLUDE_DIRS} )
ADD_PARAVIEW_PLUGIN(MantidParaViewSplatterPlotSMPlugin "1.0"
	SERVER_MANAGER_XML SplatterPlot.xml
	SERVER_MANAGER_SOURCES vtkSplatterPlot.cxx
	GUI_RESOURCES SplatterPlot.qrc
	GUI_RESOURCE_FILES SplatterPlotGUI.xml)
	
# Add to the 'VatesParaViewPlugins' group in VS
set_property( TARGET MantidParaViewSplatterPlotSMPlugin PROPERTY FOLDER "MantidVatesParaViewPlugins" )

include_directories ( SYSTEM ${QWT_INCLUDE_DIR} )

target_link_libraries( MantidParaViewSplatterPlotSMPlugin LINK_PUBLIC 
${MANTID_SUBPROJECT_LIBS} ${QWT_LIBRARIES})

if (OSX_VERSION VERSION_GREATER 10.8)
  set_target_properties ( MantidParaViewSplatterPlotSMPlugin PROPERTIES INSTALL_RPATH "@loader_path/../../Contents/Libraries;@loader_path/../../Contents/MacOS")
endif ()

# Put library into subfolder.
SET_TARGET_OUTPUT_DIRECTORY(MantidParaViewSplatterPlotSMPlugin ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}/${PVPLUGINS_DIR}/${PVPLUGINS_SUBDIR})

install( TARGETS MantidParaViewSplatterPlotSMPlugin ${SYSTEM_PACKAGE_TARGET} DESTINATION ${PVPLUGINS_DIR}/${PVPLUGINS_SUBDIR} )

