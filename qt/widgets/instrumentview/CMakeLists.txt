if(NOT ENABLE_OPENGL)
  message(
    FATAL_ERROR
      "Building instrument view with OpenGL disabled is not supported. Please set ENABLE_OPENGL=ON to continue."
  )
endif()

set(SRC_FILES
    src/BankRenderingHelpers.cpp
    src/BankTextureBuilder.cpp
    src/BinDialog.cpp
    src/CollapsibleStack.cpp
    src/DetXMLFile.cpp
    src/GLColor.cpp
    src/InstrumentActor.cpp
    src/InstrumentDisplay.cpp
    src/InstrumentTreeModel.cpp
    src/InstrumentTreeWidget.cpp
    src/InstrumentWidget.cpp
    src/InstrumentWidgetEncoder.cpp
    src/InstrumentWidgetDecoder.cpp
    src/InstrumentWidgetMaskTab.cpp
    src/InstrumentWidgetPickTab.cpp
    src/InstrumentWidgetRenderTab.cpp
    src/InstrumentRendererClassic.cpp
    src/InstrumentRendererMultiList.cpp
    src/InstrumentRenderer.cpp
    src/InstrumentWidgetTab.cpp
    src/InstrumentWidgetTreeTab.cpp
    src/GLDisplay.cpp
    src/MaskBinsData.cpp
    src/MiniPlotMpl.cpp
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
    src/QtDisplay.cpp
    src/UCorrectionDialog.cpp
    src/UnwrappedCylinder.cpp
    src/UnwrappedDetector.cpp
    src/UnwrappedSphere.cpp
    src/UnwrappedSurface.cpp
    src/Viewport.cpp
    src/XIntegrationControl.cpp
)

set(MOC_FILES
    inc/MantidQtWidgets/InstrumentView/BinDialog.h
    inc/MantidQtWidgets/InstrumentView/CollapsibleStack.h
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
    inc/MantidQtWidgets/InstrumentView/GLDisplay.h
    inc/MantidQtWidgets/InstrumentView/MiniPlotMpl.h
    inc/MantidQtWidgets/InstrumentView/PeakOverlay.h
    inc/MantidQtWidgets/InstrumentView/Projection3D.h
    inc/MantidQtWidgets/InstrumentView/ProjectionSurface.h
    inc/MantidQtWidgets/InstrumentView/Shape2DCollection.h
    inc/MantidQtWidgets/InstrumentView/UCorrectionDialog.h
    inc/MantidQtWidgets/InstrumentView/UnwrappedSurface.h
    inc/MantidQtWidgets/InstrumentView/XIntegrationControl.h
)

set(INC_FILES
    inc/MantidQtWidgets/InstrumentView/BankRenderingHelpers.h
    inc/MantidQtWidgets/InstrumentView/BankTextureBuilder.h
    inc/MantidQtWidgets/InstrumentView/BinDialog.h
    inc/MantidQtWidgets/InstrumentView/CollapsibleStack.h
    inc/MantidQtWidgets/InstrumentView/ColorMap.h
    inc/MantidQtWidgets/InstrumentView/DetXMLFile.h
    inc/MantidQtWidgets/InstrumentView/DllOption.h
    inc/MantidQtWidgets/InstrumentView/GLColor.h
    inc/MantidQtWidgets/InstrumentView/GLDisplay.h
    inc/MantidQtWidgets/InstrumentView/GridTextureFace.h
    inc/MantidQtWidgets/InstrumentView/IGLDisplay.h
    inc/MantidQtWidgets/InstrumentView/IStackedLayout.h
    inc/MantidQtWidgets/InstrumentView/IInstrumentDisplay.h
    inc/MantidQtWidgets/InstrumentView/IQtDisplay.h
    inc/MantidQtWidgets/InstrumentView/InstrumentActor.h
    inc/MantidQtWidgets/InstrumentView/InstrumentDisplay.h
    inc/MantidQtWidgets/InstrumentView/InstrumentTreeModel.h
    inc/MantidQtWidgets/InstrumentView/InstrumentTreeWidget.h
    inc/MantidQtWidgets/InstrumentView/InstrumentWidget.h
    inc/MantidQtWidgets/InstrumentView/InstrumentWidgetEncoder.h
    inc/MantidQtWidgets/InstrumentView/InstrumentWidgetDecoder.h
    inc/MantidQtWidgets/InstrumentView/InstrumentRendererClassic.h
    inc/MantidQtWidgets/InstrumentView/InstrumentRendererMultiList.h
    inc/MantidQtWidgets/InstrumentView/InstrumentRenderer.h
    inc/MantidQtWidgets/InstrumentView/InstrumentWidgetMaskTab.h
    inc/MantidQtWidgets/InstrumentView/InstrumentWidgetPickTab.h
    inc/MantidQtWidgets/InstrumentView/InstrumentWidgetRenderTab.h
    inc/MantidQtWidgets/InstrumentView/InstrumentWidgetTab.h
    inc/MantidQtWidgets/InstrumentView/InstrumentWidgetTreeTab.h
    inc/MantidQtWidgets/InstrumentView/InstrumentWidgetTypes.h
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
    inc/MantidQtWidgets/InstrumentView/StackedLayout.h
    inc/MantidQtWidgets/InstrumentView/QtDisplay.h
    inc/MantidQtWidgets/InstrumentView/UCorrectionDialog.h
    inc/MantidQtWidgets/InstrumentView/UnwrappedCylinder.h
    inc/MantidQtWidgets/InstrumentView/UnwrappedDetector.h
    inc/MantidQtWidgets/InstrumentView/UnwrappedSphere.h
    inc/MantidQtWidgets/InstrumentView/UnwrappedSurface.h
    inc/MantidQtWidgets/InstrumentView/Viewport.h
    inc/MantidQtWidgets/InstrumentView/XIntegrationControl.h
)

set(UI_FILES inc/MantidQtWidgets/InstrumentView/UCorrectionDialog.ui)

# Target
mtd_add_qt_library(
  TARGET_NAME MantidQtWidgetsInstrumentView
  QT_VERSION 5
  SRC ${SRC_FILES}
  MOC ${MOC_FILES}
  NOMOC ${INC_FILES}
  UI ${UI_FILES}
  RES ../../../images/instrumentview.qrc
  DEFS IN_MANTIDQT_INSTRUMENTVIEW
  INCLUDE_DIRS inc
  LINK_LIBS Mantid::DataObjects ${CORE_MANTIDLIBS} Mantid::PythonInterfaceCore ${POCO_LIBRARIES} ${OPENGL_gl_LIBRARY}
            ${OPENGL_glu_LIBRARY}
  QT5_LINK_LIBS Qt5::OpenGL
  MTD_QT_LINK_LIBS MantidQtWidgetsCommon MantidQtWidgetsPlotting MantidQtWidgetsMplCpp
  INSTALL_DIR ${WORKBENCH_LIB_DIR}
  OSX_INSTALL_RPATH @loader_path/../MacOS @loader_path/../Frameworks
  LINUX_INSTALL_RPATH "\$ORIGIN/../${WORKBENCH_LIB_DIR}"
)

# Testing target
if(MANTID_FRAMEWORK_LIB STREQUAL "BUILD")
  add_subdirectory(test)
endif()
