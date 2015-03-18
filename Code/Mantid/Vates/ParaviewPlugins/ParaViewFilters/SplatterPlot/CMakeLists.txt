PROJECT(SplatterPlot)

ADD_PARAVIEW_PLUGIN(MantidParaViewSplatterPlotSMPlugin "1.0"
	SERVER_MANAGER_XML SplatterPlot.xml
	SERVER_MANAGER_SOURCES vtkSplatterPlot.cxx
	GUI_RESOURCES SplatterPlot.qrc
	GUI_RESOURCE_FILES SplatterPlotGUI.xml)
	
# Add to the 'VatesParaViewPlugins' group in VS
set_property( TARGET MantidParaViewSplatterPlotSMPlugin PROPERTY FOLDER "MantidVatesParaViewPlugins" )

include_directories ( SYSTEM ${QWT_INCLUDE_DIR} )

target_link_libraries( MantidParaViewSplatterPlotSMPlugin 
${MANTID_SUBPROJECT_LIBS} ${QWT_LIBRARIES})

# Put library into subfolder.
SET_TARGET_OUTPUT_DIRECTORY(MantidParaViewSplatterPlotSMPlugin ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}/${PVPLUGINS_DIR}/${PVPLUGINS_SUBDIR})

install( TARGETS MantidParaViewSplatterPlotSMPlugin ${SYSTEM_PACKAGE_TARGET} DESTINATION ${PVPLUGINS_DIR}/${PVPLUGINS_SUBDIR} )

