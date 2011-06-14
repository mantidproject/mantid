cmake_minimum_required( VERSION 2.6)
project( MantidParaViewRebinningCutter )

add_paraview_plugin( MantidParaViewRebinningCutterSMPlugin "1.0"
	SERVER_MANAGER_XML RebinningCutter.xml
	SERVER_MANAGER_SOURCES vtkRebinningCutter.cxx
        GUI_RESOURCES RebinningCutter.qrc
		GUI_RESOURCE_FILES RebinningCutterGUI.xml
)

target_link_libraries( MantidParaViewRebinningCutterSMPlugin 
${MANTID_SUBPROJECT_LIBS} )

# Put library into subfolder.
#output_binary_to(MantidParaViewRebinningCutterSMPlugin PVPlugins)

# Windows-only at the moment.
if( ${CMAKE_SYSTEM_NAME} STREQUAL "Windows" )
  install(TARGETS MantidParaViewRebinningCutterSMPlugin RUNTIME DESTINATION ${LIB_DIR}/PVPlugins)
endif( ${CMAKE_SYSTEM_NAME} STREQUAL "Windows" )

