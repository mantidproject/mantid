cmake_minimum_required( VERSION 2.8.12 )
project( MantidParaViewMDEWRebinningCutter )

add_paraview_plugin( MantidParaViewMDEWRebinningCutterSMPlugin "1.0"
	SERVER_MANAGER_XML MDEWRebinningCutter.xml
	SERVER_MANAGER_SOURCES vtkMDEWRebinningCutter.cxx
        GUI_RESOURCES MDEWRebinningCutter.qrc
		GUI_RESOURCE_FILES MDEWRebinningCutterGUI.xml
)

# Add to the 'VatesParaViewPlugins' group in VS
set_property( TARGET MantidParaViewMDEWRebinningCutterSMPlugin PROPERTY FOLDER "MantidVatesParaViewPlugins" )

target_link_libraries( MantidParaViewMDEWRebinningCutterSMPlugin 
${MANTID_SUBPROJECT_LIBS} )

# Put library into subfolder.
SET_TARGET_OUTPUT_DIRECTORY(MantidParaViewMDEWRebinningCutterSMPlugin  ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}/${PVPLUGINS_DIR}/${PVPLUGINS_SUBDIR})

install( TARGETS MantidParaViewMDEWRebinningCutterSMPlugin ${SYSTEM_PACKAGE_TARGET} DESTINATION ${PVPLUGINS_DIR}/${PVPLUGINS_SUBDIR} )

