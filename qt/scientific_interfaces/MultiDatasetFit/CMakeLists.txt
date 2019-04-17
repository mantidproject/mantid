set ( SRC_FILES
	MDFAddWorkspaceDialog.cpp
	MDFDataController.cpp
	MDFDatasetPlotData.cpp
	MDFEditLocalParameterDialog.cpp
	MDFFunctionPlotData.cpp
	MDFLocalParameterEditor.cpp
	MDFLocalParameterItemDelegate.cpp
	MDFLogValueFinder.cpp
	MDFPlotController.cpp
	MultiDatasetFit.cpp
)

# Include files aren't required, but this makes them appear in Visual Studio
# IMPORTANT: Include files are required in the MOC_FILES set. Scroll down to find it.
set ( INC_FILES
    DllConfig.h
	MDFAddWorkspaceDialog.h
	MDFDataController.h
	MDFDatasetPlotData.h
	MDFEditLocalParameterDialog.h
	MDFFunctionPlotData.h
	MDFLocalParameterEditor.h
	MDFLocalParameterItemDelegate.h
	MDFLogValueFinder.h
	MDFPlotController.h
	MultiDatasetFit.h
)

set ( MOC_FILES
    MultiDatasetFit.h
    MDFAddWorkspaceDialog.h
    MDFDataController.h
    MDFPlotController.h
    MDFLocalParameterEditor.h
    MDFLocalParameterItemDelegate.h
    MDFEditLocalParameterDialog.h
)

set ( UI_FILES
   MultiDatasetFit.ui
   MDFAddWorkspaceDialog.ui
   EditLocalParameterDialog.ui
)

mtd_add_qt_library (TARGET_NAME MantidScientificInterfacesMultiDatasetFit
  QT_VERSION 4
  SRC ${SRC_FILES}
  MOC ${MOC_FILES}
  NOMOC ${INC_FILES}
  UI ${UI_FILES}
  RES ../../resources/icons/MultiDatasetFitIcons.qrc
  DEFS IN_MANTIDQT_MULTIDATASETFIT
  PRECOMPILED PrecompiledHeader.h
  INCLUDE_DIRS
    ..
  SYSTEM_INCLUDE_DIRS
    ${Boost_INCLUDE_DIRS}
  LINK_LIBS
    ${TCMALLOC_LIBRARIES_LINKTIME}
    ${CORE_MANTIDLIBS}
    ${POCO_LIBRARIES}
    ${Boost_LIBRARIES}
    ${JSONCPP_LIBRARIES}
  QT4_LINK_LIBS
      Qwt5
  MTD_QT_LINK_LIBS
    MantidQtWidgetsCommon
    MantidQtWidgetsPlotting
  INSTALL_DIR_BASE
    ${PLUGINS_DIR}
  OSX_INSTALL_RPATH
    @loader_path/../../Contents/MacOS
  LINUX_INSTALL_RPATH
    "\$ORIGIN/../../${LIB_DIR}"
)
