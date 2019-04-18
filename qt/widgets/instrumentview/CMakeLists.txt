set ( SRC_FILES
  src/BankRenderingHelpers.cpp
  src/BankTextureBuilder.cpp
  src/BinDialog.cpp
  src/CollapsiblePanel.cpp
  src/DetXMLFile.cpp
  src/GLColor.cpp
  src/GLObject.cpp
  src/InstrumentActor.cpp
  src/InstrumentTreeModel.cpp
  src/InstrumentTreeWidget.cpp
  src/InstrumentWidget.cpp
  src/InstrumentWidgetEncoder.cpp
  src/InstrumentWidgetDecoder.cpp
  src/InstrumentWidgetMaskTab.cpp
  src/InstrumentWidgetPickTab.cpp
  src/InstrumentWidgetRenderTab.cpp
  src/InstrumentRenderer.cpp
  src/InstrumentWidgetTab.cpp
  src/InstrumentWidgetTreeTab.cpp
  src/MantidGLWidget.cpp
  src/MaskBinsData.cpp
  src/OpenGLError.cpp
  src/PanelsSurface.cpp
  src/PeakMarker2D.cpp
  src/PeakOverlay.cpp
  src/Projection3D.cpp
  src/ProjectionSurface.cpp
  src/RectF.cpp
  src/RotationSurface.cpp
  src/Shape2D.cpp
  src/Shape2DCollection.cpp
  src/SimpleWidget.cpp
  src/UCorrectionDialog.cpp
  src/UnwrappedCylinder.cpp
  src/UnwrappedDetector.cpp
  src/UnwrappedSphere.cpp
  src/UnwrappedSurface.cpp
  src/Viewport.cpp
  src/XIntegrationControl.cpp
)

set ( QT4_SRC_FILES
  src/MiniPlotQwt.cpp
)

set ( QT5_SRC_FILES
  src/MiniPlotMpl.cpp
)

set ( MOC_FILES
  inc/MantidQtWidgets/InstrumentView/BinDialog.h
  inc/MantidQtWidgets/InstrumentView/CollapsiblePanel.h
  inc/MantidQtWidgets/InstrumentView/InstrumentActor.h
  inc/MantidQtWidgets/InstrumentView/InstrumentTreeModel.h
  inc/MantidQtWidgets/InstrumentView/InstrumentTreeWidget.h
  inc/MantidQtWidgets/InstrumentView/InstrumentWidget.h
  inc/MantidQtWidgets/InstrumentView/InstrumentWidgetDecoder.h
  inc/MantidQtWidgets/InstrumentView/InstrumentWidgetMaskTab.h
  inc/MantidQtWidgets/InstrumentView/InstrumentWidgetPickTab.h
  inc/MantidQtWidgets/InstrumentView/InstrumentWidgetRenderTab.h
  inc/MantidQtWidgets/InstrumentView/InstrumentWidgetTab.h
  inc/MantidQtWidgets/InstrumentView/InstrumentWidgetTreeTab.h
  inc/MantidQtWidgets/InstrumentView/MantidGLWidget.h
  inc/MantidQtWidgets/InstrumentView/PeakOverlay.h
  inc/MantidQtWidgets/InstrumentView/Projection3D.h
  inc/MantidQtWidgets/InstrumentView/ProjectionSurface.h
  inc/MantidQtWidgets/InstrumentView/Shape2DCollection.h
  inc/MantidQtWidgets/InstrumentView/UCorrectionDialog.h
  inc/MantidQtWidgets/InstrumentView/UnwrappedSurface.h
  inc/MantidQtWidgets/InstrumentView/XIntegrationControl.h
)

set ( QT4_MOC_FILES
  inc/MantidQtWidgets/InstrumentView/MiniPlotQwt.h
)

set ( QT5_MOC_FILES
  inc/MantidQtWidgets/InstrumentView/MiniPlotMpl.h
)

set ( INC_FILES
  inc/MantidQtWidgets/InstrumentView/BankRenderingHelpers.h
  inc/MantidQtWidgets/InstrumentView/BankTextureBuilder.h
  inc/MantidQtWidgets/InstrumentView/BinDialog.h
  inc/MantidQtWidgets/InstrumentView/CollapsiblePanel.h
  inc/MantidQtWidgets/InstrumentView/ColorMap.h
  inc/MantidQtWidgets/InstrumentView/DetXMLFile.h
  inc/MantidQtWidgets/InstrumentView/DllOption.h
  inc/MantidQtWidgets/InstrumentView/GLColor.h
  inc/MantidQtWidgets/InstrumentView/GLObject.h
  inc/MantidQtWidgets/InstrumentView/GridTextureFace.h
  inc/MantidQtWidgets/InstrumentView/InstrumentActor.h
  inc/MantidQtWidgets/InstrumentView/InstrumentTreeModel.h
  inc/MantidQtWidgets/InstrumentView/InstrumentTreeWidget.h
  inc/MantidQtWidgets/InstrumentView/InstrumentWidget.h
  inc/MantidQtWidgets/InstrumentView/InstrumentWidgetEncoder.h
  inc/MantidQtWidgets/InstrumentView/InstrumentWidgetDecoder.h
  inc/MantidQtWidgets/InstrumentView/InstrumentRenderer.h
  inc/MantidQtWidgets/InstrumentView/InstrumentWidgetMaskTab.h
  inc/MantidQtWidgets/InstrumentView/InstrumentWidgetPickTab.h
  inc/MantidQtWidgets/InstrumentView/InstrumentWidgetRenderTab.h
  inc/MantidQtWidgets/InstrumentView/InstrumentWidgetTab.h
  inc/MantidQtWidgets/InstrumentView/InstrumentWidgetTreeTab.h
  inc/MantidQtWidgets/InstrumentView/InstrumentWidgetTypes.h
  inc/MantidQtWidgets/InstrumentView/MantidGLWidget.h
  inc/MantidQtWidgets/InstrumentView/MaskBinsData.h
  inc/MantidQtWidgets/InstrumentView/OpenGLError.h
  inc/MantidQtWidgets/InstrumentView/PanelsSurface.h
  inc/MantidQtWidgets/InstrumentView/PeakMarker2D.h
  inc/MantidQtWidgets/InstrumentView/PeakOverlay.h
  inc/MantidQtWidgets/InstrumentView/Projection3D.h
  inc/MantidQtWidgets/InstrumentView/ProjectionSurface.h
  inc/MantidQtWidgets/InstrumentView/RectF.h
  inc/MantidQtWidgets/InstrumentView/RotationSurface.h
  inc/MantidQtWidgets/InstrumentView/Shape2D.h
  inc/MantidQtWidgets/InstrumentView/Shape2DCollection.h
  inc/MantidQtWidgets/InstrumentView/SimpleWidget.h
  inc/MantidQtWidgets/InstrumentView/UCorrectionDialog.h
  inc/MantidQtWidgets/InstrumentView/UnwrappedCylinder.h
  inc/MantidQtWidgets/InstrumentView/UnwrappedDetector.h
  inc/MantidQtWidgets/InstrumentView/UnwrappedSphere.h
  inc/MantidQtWidgets/InstrumentView/UnwrappedSurface.h
  inc/MantidQtWidgets/InstrumentView/Viewport.h
  inc/MantidQtWidgets/InstrumentView/XIntegrationControl.h
)

set ( UI_FILES
  inc/MantidQtWidgets/InstrumentView/UCorrectionDialog.ui
)

# Target
mtd_add_qt_library (TARGET_NAME MantidQtWidgetsInstrumentView
  QT_VERSION 4
  SRC
    ${SRC_FILES}
    ${QT4_SRC_FILES}
  MOC
    ${MOC_FILES}
    ${QT4_MOC_FILES}
  NOMOC
    ${INC_FILES}
    ${QT4_INC_FILES}
  UI ${UI_FILES}
  RES
    ../../../images/instrumentview.qrc
  DEFS
    IN_MANTIDQT_INSTRUMENTVIEW
  INCLUDE_DIRS
    inc
  SYSTEM_INCLUDE_DIRS
    ${Boost_INCLUDE_DIRS}
  LINK_LIBS
    ${TCMALLOC_LIBRARIES_LINKTIME}
    ${CORE_MANTIDLIBS}
    ${POCO_LIBRARIES}
    ${Boost_LIBRARIES}
    ${PYTHON_LIBRARIES}
    ${OPENGL_gl_LIBRARY}
    ${OPENGL_glu_LIBRARY}
  QT4_LINK_LIBS
    Qt4::QtOpenGL
    Qwt5
  MTD_QT_LINK_LIBS
    MantidQtWidgetsCommon
    MantidQtWidgetsPlotting
  INSTALL_DIR
    ${LIB_DIR}
  OSX_INSTALL_RPATH
    @loader_path/../MacOS
    @loader_path/../Libraries
  LINUX_INSTALL_RPATH
    "\$ORIGIN/../${LIB_DIR}"
)

find_package ( BoostPython REQUIRED )
mtd_add_qt_library (TARGET_NAME MantidQtWidgetsInstrumentView
  QT_VERSION 5
  SRC
    ${SRC_FILES}
    ${QT5_SRC_FILES}
  MOC
    ${MOC_FILES}
    ${QT5_MOC_FILES}
  NOMOC
    ${INC_FILES}
    ${QT5_INC_FILES}
  UI ${UI_FILES}
  RES
    ../../../images/instrumentview.qrc
  DEFS
    IN_MANTIDQT_INSTRUMENTVIEW
  INCLUDE_DIRS
    inc
  LINK_LIBS
    ${TCMALLOC_LIBRARIES_LINKTIME}
    ${CORE_MANTIDLIBS}
    PythonInterfaceCore
    ${POCO_LIBRARIES}
    ${Boost_LIBRARIES}
    ${PYTHON_LIBRARIES}
    ${OPENGL_gl_LIBRARY}
    ${OPENGL_glu_LIBRARY}
  QT5_LINK_LIBS
    Qt5::OpenGL
  MTD_QT_LINK_LIBS
    MantidQtWidgetsCommon
    MantidQtWidgetsMplCpp
  INSTALL_DIR
    ${LIB_DIR}
    ${WORKBENCH_LIB_DIR}
  OSX_INSTALL_RPATH
    @loader_path/../MacOS
    @loader_path/../Libraries
  LINUX_INSTALL_RPATH
    "\$ORIGIN/../${WORKBENCH_LIB_DIR}"
)

###########################################################################
# Testing
###########################################################################
set( TEST_FILES
  test/InstrumentWidgetDecoderTest.h
  test/InstrumentWidgetEncoderTest.h
)

mtd_add_qt_tests (TARGET_NAME MantidQtWidgetsInstrumentWidgetTest
  QT_VERSION 5
  SRC ${TEST_FILES}
  INCLUDE_DIRS
    ../../../Framework/DataObjects/inc
    ../../../Framework/TestHelpers/inc
  TEST_HELPER_SRCS
    ../../../Framework/TestHelpers/src/TearDownWorld.cpp
    ../../../Framework/TestHelpers/src/ComponentCreationHelper.cpp
    ../../../Framework/TestHelpers/src/InstrumentCreationHelper.cpp
    ../../../Framework/TestHelpers/src/WorkspaceCreationHelper.cpp
  LINK_LIBS
    ${CORE_MANTIDLIBS}
    PythonInterfaceCore
    DataObjects
    ${POCO_LIBRARIES}
    ${Boost_LIBRARIES}
    ${PYTHON_LIBRARIES}
    ${GMOCK_LIBRARIES}
    ${GTEST_LIBRARIES}
  QT5_LINK_LIBS
    Qt5::OpenGL
  MTD_QT_LINK_LIBS
    MantidQtWidgetsCommon
    MantidQtWidgetsInstrumentView
    MantidQtWidgetsMplCpp
  PARENT_DEPENDENCIES
    GUITests
)
