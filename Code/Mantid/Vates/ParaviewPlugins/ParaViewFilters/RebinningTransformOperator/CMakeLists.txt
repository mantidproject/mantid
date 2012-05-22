project( MantidParaViewMDEWRebinningCutter )

add_paraview_plugin( MantidParaViewRebinningTransformSMPlugin "1.0"
	SERVER_MANAGER_XML RebinningTransformOperator.xml
	SERVER_MANAGER_SOURCES vtkRebinningTransformOperator.cxx
        GUI_RESOURCES RebinningTransformOperator.qrc
		GUI_RESOURCE_FILES RebinningTransformOperatorGUI.xml
)

# Add to the 'VatesParaViewPlugins' group in VS
set_property( TARGET MantidParaViewRebinningTransformSMPlugin PROPERTY FOLDER "MantidVatesParaViewPlugins" )

target_link_libraries( MantidParaViewRebinningTransformSMPlugin 
${MANTID_SUBPROJECT_LIBS} )

# Put library into subfolder.
SET_TARGET_OUTPUT_DIRECTORY(MantidParaViewRebinningTransformSMPlugin  ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}/${PVPLUGINS_DIR}/${PVPLUGINS_SUBDIR})

install( TARGETS MantidParaViewRebinningTransformSMPlugin ${SYSTEM_PACKAGE_TARGET} DESTINATION ${PVPLUGINS_DIR}/${PVPLUGINS_SUBDIR} )

