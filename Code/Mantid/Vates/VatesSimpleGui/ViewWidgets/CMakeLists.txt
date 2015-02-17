project( MantidVatesSimpleGuiViewWidgets )

# These are the C++ files to be compiled.
set( INCLUDE_FILES
  inc/MantidVatesSimpleGuiViewWidgets/AutoScaleRangeGenerator.h
  inc/MantidVatesSimpleGuiViewWidgets/CameraManager.h
  inc/MantidVatesSimpleGuiViewWidgets/ColorSelectionWidget.h
  inc/MantidVatesSimpleGuiViewWidgets/ColorUpdater.h
  inc/MantidVatesSimpleGuiViewWidgets/LibHelper.h
  inc/MantidVatesSimpleGuiViewWidgets/MdViewerWidget.h
  inc/MantidVatesSimpleGuiViewWidgets/MultisliceView.h
  inc/MantidVatesSimpleGuiViewWidgets/PeakViewerVsi.h
  inc/MantidVatesSimpleGuiViewWidgets/PeaksWidget.h
  inc/MantidVatesSimpleGuiViewWidgets/SaveScreenshotReaction.h
  inc/MantidVatesSimpleGuiViewWidgets/StandardView.h
  inc/MantidVatesSimpleGuiViewWidgets/SplatterPlotView.h
  inc/MantidVatesSimpleGuiViewWidgets/ThreesliceView.h
  inc/MantidVatesSimpleGuiViewWidgets/TimeControlWidget.h
  inc/MantidVatesSimpleGuiViewWidgets/ViewBase.h
  inc/MantidVatesSimpleGuiViewWidgets/WidgetDllOption.h
)

set( SOURCE_FILES
  src/AutoScaleRangeGenerator.cpp
  src/CameraManager.cpp
  src/ColorSelectionWidget.cpp
  src/ColorUpdater.cpp
  src/MdViewerWidget.cpp
  src/MultisliceView.cpp
  src/PeakViewerVsi.cpp
  src/PeaksWidget.cpp
  src/SaveScreenshotReaction.cpp
  src/StandardView.cpp
  src/SplatterPlotView.cpp
  src/ThreesliceView.cpp
  src/TimeControlWidget.cpp
  src/ViewBase.cpp
)

# These are the headers to be preprocessed using
# Qt's moc preprocessor.
qt4_wrap_cpp( MOC_SOURCES
  inc/MantidVatesSimpleGuiViewWidgets/ColorSelectionWidget.h
  inc/MantidVatesSimpleGuiViewWidgets/MdViewerWidget.h
  inc/MantidVatesSimpleGuiViewWidgets/MultisliceView.h
  inc/MantidVatesSimpleGuiViewWidgets/PeakViewerVsi.h
  inc/MantidVatesSimpleGuiViewWidgets/PeaksWidget.h
  inc/MantidVatesSimpleGuiViewWidgets/SaveScreenshotReaction.h
  inc/MantidVatesSimpleGuiViewWidgets/StandardView.h
  inc/MantidVatesSimpleGuiViewWidgets/SplatterPlotView.h
  inc/MantidVatesSimpleGuiViewWidgets/ThreesliceView.h
  inc/MantidVatesSimpleGuiViewWidgets/TimeControlWidget.h
  inc/MantidVatesSimpleGuiViewWidgets/ViewBase.h
)

# These are the ui files to be processed using
# Qt's ui file processor.
qt4_wrap_ui( UI_BUILT_SOURCES
  inc/MantidVatesSimpleGuiViewWidgets/ColorSelectionWidget.ui
  inc/MantidVatesSimpleGuiViewWidgets/MdViewerWidget.ui
  inc/MantidVatesSimpleGuiViewWidgets/MultisliceView.ui
  inc/MantidVatesSimpleGuiViewWidgets/PeaksWidget.ui
  inc/MantidVatesSimpleGuiViewWidgets/StandardView.ui
  inc/MantidVatesSimpleGuiViewWidgets/SplatterPlotView.ui
  inc/MantidVatesSimpleGuiViewWidgets/ThreesliceView.ui
  inc/MantidVatesSimpleGuiViewWidgets/TimeControlWidget.ui
)

# Add the QtWidgets icons
qt4_add_resources( RES_FILES icons/ViewWidgetsIcons.qrc )

# Setup the include locations.
include_directories(
  inc
  ${CMAKE_SOURCE_DIR}/MantidQt/API/inc
  ${CMAKE_SOURCE_DIR}/MantidQt/MantidWidgets/inc
  ${CMAKE_SOURCE_DIR}/MantidQt/SliceViewer/inc
  ${CMAKE_BINARY_DIR}/MantidQt/SliceViewer
  ${CMAKE_SOURCE_DIR}/MantidQt/Factory/inc
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}/../QtWidgets
  ${QWT_INCLUDE_DIR}
)

set( ALL_FILES
${INCLUDE_FILES}
${SOURCE_FILES}
${MOC_SOURCES}
${UI_BUILT_SOURCES}
${RES_FILES}
)

set( TEST_SUITES
    test/suite_MDEventWorkspace
    test/suite_MDHistoWorkspace
)

# For Windows
add_definitions( -DIN_MANTID_VATES_SIMPLEGUI_VIEWWIDGETS )
# Add the target for this directory
add_library( VatesSimpleGuiViewWidgets ${ALL_FILES} )
# Set the name of the generated library
set_target_properties( VatesSimpleGuiViewWidgets PROPERTIES OUTPUT_NAME MantidVatesSimpleGuiViewWidgets )
# Add to the 'VatesSimpleGui' group in VS
set_property( TARGET VatesSimpleGuiViewWidgets PROPERTY FOLDER MantidVatesSimpleGui )

target_link_libraries( VatesSimpleGuiViewWidgets
VatesSimpleGuiQtWidgets
pqApplicationComponents
pqComponents
${QT_LIBRARIES}
MantidQtAPI
VatesAPI
${MANTID_SUBPROJECT_LIBS}
MantidQtSliceViewer
MantidQtFactory
)

configure_file ( ${CMAKE_CURRENT_SOURCE_DIR}/inc/MantidVatesSimpleGuiViewWidgets/LibHelper.h.in
                 ${CMAKE_CURRENT_SOURCE_DIR}/inc/MantidVatesSimpleGuiViewWidgets/LibHelper.h )

if( SQUISH_FOUND )
  # Need to set the AUT first
  set( SQUISH_AUT MantidPlot )
  set( SQUISH_AUT_PATH $<TARGET_FILE_DIR:${SQUISH_AUT}> )

  # Need to set environmental variables next
  set( SQUISH_ENV_VARS
    PV_PLUGIN_PATH=$<TARGET_FILE_DIR:${SQUISH_AUT}>/${PVPLUGINS_DIR}/${PVPLUGINS_DIR}
    SCRIPTS_DIR=${CMAKE_CURRENT_SOURCE_DIR}/test/mp_scripts
    SQUISH_SCRIPT_DIR=${VSI_GLOBAL_SCRIPTS}:${CMAKE_CURRENT_SOURCE_DIR}/test/common/scripts
  )
  squish_add_test_suite( ${TEST_SUITES} )

endif()

install( TARGETS VatesSimpleGuiViewWidgets ${SYSTEM_PACKAGE_TARGET} DESTINATION ${PVPLUGINS_DIR} )
