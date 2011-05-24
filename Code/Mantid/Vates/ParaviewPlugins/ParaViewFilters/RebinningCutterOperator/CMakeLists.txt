cmake_minimum_required( VERSION 2.6)
project( MantidParaViewRebinningCutter )

set_mantid_subprojects( Vates/VatesAPI )

add_paraview_plugin( MantidParaViewRebinningCutterSMPlugin "1.0"
	SERVER_MANAGER_XML RebinningCutter.xml
	SERVER_MANAGER_SOURCES vtkRebinningCutter.cxx
        GUI_RESOURCES RebinningCutter.qrc
		GUI_RESOURCE_FILES RebinningCutterGUI.xml
)

target_link_libraries( MantidParaViewRebinningCutterSMPlugin 
${MANTID_SUBPROJECT_LIBS} )
