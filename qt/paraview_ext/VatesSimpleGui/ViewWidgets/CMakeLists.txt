project(MantidVatesSimpleGuiViewWidgets)

# These are the C++ files to be compiled.
set(INCLUDE_FILES
    inc/MantidVatesSimpleGuiViewWidgets/AutoScaleRangeGenerator.h
    inc/MantidVatesSimpleGuiViewWidgets/CameraManager.h
    inc/MantidVatesSimpleGuiViewWidgets/BackgroundRgbProvider.h
    inc/MantidVatesSimpleGuiViewWidgets/ColorMapEditorPanel.h
    inc/MantidVatesSimpleGuiViewWidgets/ColorSelectionWidget.h
    inc/MantidVatesSimpleGuiViewWidgets/ColorUpdater.h
    inc/MantidVatesSimpleGuiViewWidgets/MdViewerWidget.h
    inc/MantidVatesSimpleGuiViewWidgets/MultisliceView.h
    inc/MantidVatesSimpleGuiViewWidgets/pqCameraToolbarNonOrthogonalAxes.h
    inc/MantidVatesSimpleGuiViewWidgets/pqCameraReactionNonOrthogonalAxes.h
    inc/MantidVatesSimpleGuiViewWidgets/VatesParaViewApplication.h
    inc/MantidVatesSimpleGuiViewWidgets/RebinAlgorithmDialogProvider.h
    inc/MantidVatesSimpleGuiViewWidgets/PeaksTableControllerVsi.h
    inc/MantidVatesSimpleGuiViewWidgets/PeaksWidget.h
    inc/MantidVatesSimpleGuiViewWidgets/PeaksTabWidget.h
    inc/MantidVatesSimpleGuiViewWidgets/RebinnedSourcesManager.h
    inc/MantidVatesSimpleGuiViewWidgets/VisibleAxesColor.h
    inc/MantidVatesSimpleGuiViewWidgets/StandardView.h
    inc/MantidVatesSimpleGuiViewWidgets/SplatterPlotView.h
    inc/MantidVatesSimpleGuiViewWidgets/ThreesliceView.h
    inc/MantidVatesSimpleGuiViewWidgets/TimeControlWidget.h
    inc/MantidVatesSimpleGuiViewWidgets/ViewBase.h
    inc/MantidVatesSimpleGuiViewWidgets/WidgetDllOption.h
    inc/MantidVatesSimpleGuiViewWidgets/VsiApplyBehaviour.h)

set(SOURCE_FILES
    src/AutoScaleRangeGenerator.cpp
    src/CameraManager.cpp
    src/BackgroundRgbProvider.cpp
    src/ColorMapEditorPanel.cpp
    src/ColorSelectionWidget.cpp
    src/ColorUpdater.cpp
    src/MdViewerWidget.cpp
    src/MultisliceView.cpp
    src/RebinAlgorithmDialogProvider.cpp
    src/PeaksTableControllerVsi.cpp
    src/PeaksTabWidget.cpp
    src/PeaksWidget.cpp
    src/pqCameraToolbarNonOrthogonalAxes.cpp
    src/pqCameraReactionNonOrthogonalAxes.cpp
    src/RebinnedSourcesManager.cpp
    src/VisibleAxesColor.cpp
    src/StandardView.cpp
    src/SplatterPlotView.cpp
    src/ThreesliceView.cpp
    src/TimeControlWidget.cpp
    src/VatesParaViewApplication.cpp
    src/ViewBase.cpp
    src/VsiApplyBehaviour.cpp)

# These are the headers to be preprocessed using Qt's moc preprocessor.
set(MOC_FILES
    inc/MantidVatesSimpleGuiViewWidgets/ColorSelectionWidget.h
    inc/MantidVatesSimpleGuiViewWidgets/ColorMapEditorPanel.h
    inc/MantidVatesSimpleGuiViewWidgets/MdViewerWidget.h
    inc/MantidVatesSimpleGuiViewWidgets/MultisliceView.h
    inc/MantidVatesSimpleGuiViewWidgets/PeaksTableControllerVsi.h
    inc/MantidVatesSimpleGuiViewWidgets/PeaksWidget.h
    inc/MantidVatesSimpleGuiViewWidgets/PeaksTabWidget.h
    inc/MantidVatesSimpleGuiViewWidgets/pqCameraToolbarNonOrthogonalAxes.h
    inc/MantidVatesSimpleGuiViewWidgets/pqCameraReactionNonOrthogonalAxes.h
    inc/MantidVatesSimpleGuiViewWidgets/RebinnedSourcesManager.h
    inc/MantidVatesSimpleGuiViewWidgets/StandardView.h
    inc/MantidVatesSimpleGuiViewWidgets/SplatterPlotView.h
    inc/MantidVatesSimpleGuiViewWidgets/ThreesliceView.h
    inc/MantidVatesSimpleGuiViewWidgets/TimeControlWidget.h
    inc/MantidVatesSimpleGuiViewWidgets/VatesParaViewApplication.h
    inc/MantidVatesSimpleGuiViewWidgets/ViewBase.h
    inc/MantidVatesSimpleGuiViewWidgets/VsiApplyBehaviour.h)

# These are the ui files to be processed using Qt's ui file processor.
set(UI_FILES
    inc/MantidVatesSimpleGuiViewWidgets/ColorSelectionWidget.ui
    inc/MantidVatesSimpleGuiViewWidgets/ColorMapEditorPanel.ui
    inc/MantidVatesSimpleGuiViewWidgets/MdViewerWidget.ui
    inc/MantidVatesSimpleGuiViewWidgets/MultisliceView.ui
    inc/MantidVatesSimpleGuiViewWidgets/PeaksWidget.ui
    inc/MantidVatesSimpleGuiViewWidgets/PeaksTabWidget.ui
    inc/MantidVatesSimpleGuiViewWidgets/pqCameraToolbarNonOrthogonalAxes.ui
    inc/MantidVatesSimpleGuiViewWidgets/StandardView.ui
    inc/MantidVatesSimpleGuiViewWidgets/SplatterPlotView.ui
    inc/MantidVatesSimpleGuiViewWidgets/ThreesliceView.ui
    inc/MantidVatesSimpleGuiViewWidgets/TimeControlWidget.ui)

# Add the QtWidgets icons
set(RES_FILES ../../../resources/icons/ViewWidgetsIcons.qrc)

mtd_add_qt_library(
  TARGET_NAME VatesSimpleGuiViewWidgets
  QT_VERSION 4
  SRC ${SOURCE_FILES}
  MOC ${MOC_FILES}
  NOMOC ${INCLUDE_FILES}
  UI ${UI_FILES}
  RES ${RES_FILES}
  DEFS
	IN_MANTID_VATES_SIMPLEGUI_VIEWWIDGETS
  INCLUDE_DIRS
	inc
  SYSTEM_INCLUDE_DIRS
	${PARAVIEW_INCLUDE_DIRS}
  LINK_LIBS
	${TCMALLOC_LIBRARIES_LINKTIME}
	pqApplicationComponents
	pqComponents
	${vtkjsoncpp_LIBRARIES}
	vtkPVServerManagerRendering
	vtkRenderingFreeType
	vtksys
	VatesAPI
	${PYTHON_LIBRARIES}
	${POCO_LIBRARIES}
	${Boost_LIBRARIES}
	${CORE_MANTIDLIBS}
	DataObjects
  QT4_LINK_LIBS
	Qwt5
  MTD_QT_LINK_LIBS
	MantidQtWidgetsCommon
	MantidQtWidgetsPlotting
	VatesSimpleGuiQtWidgets
	MantidQtWidgetsFactory
	MantidQtWidgetsSliceViewer
  INSTALL_DIR_BASE
	${PLUGINS_DIR}
  OSX_INSTALL_RPATH
	@loader_path/../../Contents/MacOS
	@loader_path/../../Contents/Libraries
  LINUX_INSTALL_RPATH
	"\$ORIGIN/../../${LIB_DIR};\$ORIGIN/../../${LIB_DIR}/paraview-${ParaView_VERSION_MAJOR}.${ParaView_VERSION_MINOR};\$ORIGIN"
  )

# Set the name of the generated library
set_target_properties(VatesSimpleGuiViewWidgetsQt4
                      PROPERTIES OUTPUT_NAME MantidVatesSimpleGuiViewWidgetsQt4)
# Add to the 'VatesSimpleGui' group in VS
set_property(TARGET VatesSimpleGuiViewWidgetsQt4
             PROPERTY FOLDER MantidVatesSimpleGui)

if(SQUISH_FOUND)
  # Need to set the AUT first
  set(SQUISH_AUT MantidPlot)
  set(SQUISH_AUT_PATH $<TARGET_FILE_DIR:${SQUISH_AUT}>)

  # Need to set environmental variables next
  set(
    SQUISH_ENV_VARS
    PV_PLUGIN_PATH=$<TARGET_FILE_DIR:${SQUISH_AUT}>/${PVPLUGINS_DIR}/${PVPLUGINS_DIR}
    SCRIPTS_DIR=${CMAKE_CURRENT_SOURCE_DIR}/test/mp_scripts
    SQUISH_SCRIPT_DIR=${VSI_GLOBAL_SCRIPTS}:${CMAKE_CURRENT_SOURCE_DIR}/test/common/scripts
    )
  set(TEST_SUITES
      test/suite_MDEventWorkspace
      test/suite_MDHistoWorkspace)

  squish_add_test_suite(${TEST_SUITES})

endif()
