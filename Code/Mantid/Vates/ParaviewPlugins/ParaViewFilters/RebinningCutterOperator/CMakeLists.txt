cmake_minimum_required(VERSION 2.6)
PROJECT(MantidParaViewRebinningCutter)

FIND_PACKAGE(ParaView REQUIRED)
INCLUDE(${PARAVIEW_USE_FILE})

set_mantid_subprojects(Vates/VatesAPI)

ADD_PARAVIEW_PLUGIN(MantidParaViewRebinningCutterSMPlugin "1.0"
	SERVER_MANAGER_XML RebinningCutter.xml
	SERVER_MANAGER_SOURCES vtkRebinningCutter.cxx
        GUI_RESOURCES RebinningCutter.qrc
		GUI_RESOURCE_FILES RebinningCutterGUI.xml
)

TARGET_LINK_LIBRARIES(MantidParaViewRebinningCutterSMPlugin ${MANTID_SUBPROJECT_LIBS})
