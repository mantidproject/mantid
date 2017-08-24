set ( SRC_FILES
	src/BinDialog.cpp
	src/CollapsiblePanel.cpp
	src/ColorMapWidget.cpp
	src/CompAssemblyActor.cpp
	src/ComponentActor.cpp
	src/DetXMLFile.cpp
	src/GLActor.cpp
	src/GLActorCollection.cpp
	src/GLActorVisitor.cpp
	src/GLColor.cpp
	src/GLObject.cpp
	src/ICompAssemblyActor.cpp
	src/InstrumentActor.cpp
	src/InstrumentTreeModel.cpp
	src/InstrumentTreeWidget.cpp
	src/InstrumentWidget.cpp
	src/InstrumentWidgetMaskTab.cpp
	src/InstrumentWidgetPickTab.cpp
	src/InstrumentWidgetRenderTab.cpp
	src/InstrumentWidgetTab.cpp
	src/InstrumentWidgetTreeTab.cpp
	src/MantidGLWidget.cpp
	src/MaskBinsData.cpp
	src/ObjCompAssemblyActor.cpp
	src/ObjComponentActor.cpp
	src/OneCurvePlot.cpp
	src/OpenGLError.cpp
	src/PanelsSurface.cpp
	src/PeakMarker2D.cpp
	src/PeakOverlay.cpp
	src/Projection3D.cpp
	src/ProjectionSurface.cpp
	src/RectF.cpp
	src/RectangularDetectorActor.cpp
	src/RotationSurface.cpp
	src/SampleActor.cpp
	src/Shape2D.cpp
	src/Shape2DCollection.cpp
	src/SimpleWidget.cpp
	src/StructuredDetectorActor.cpp
	src/UCorrectionDialog.cpp
	src/UnwrappedCylinder.cpp
	src/UnwrappedSphere.cpp
	src/UnwrappedSurface.cpp
	src/Viewport.cpp
	src/XIntegrationControl.cpp
)

set ( MOC_FILES
  inc/MantidQtWidgets/InstrumentView/BinDialog.h
  inc/MantidQtWidgets/InstrumentView/InstrumentTreeModel.h
  inc/MantidQtWidgets/InstrumentView/InstrumentTreeWidget.h
  inc/MantidQtWidgets/InstrumentView/InstrumentWidget.h
  inc/MantidQtWidgets/InstrumentView/InstrumentWidgetTab.h
  inc/MantidQtWidgets/InstrumentView/InstrumentWidgetRenderTab.h
  inc/MantidQtWidgets/InstrumentView/InstrumentWidgetPickTab.h
  inc/MantidQtWidgets/InstrumentView/InstrumentWidgetMaskTab.h
  inc/MantidQtWidgets/InstrumentView/InstrumentWidgetTreeTab.h
  inc/MantidQtWidgets/InstrumentView/ColorMapWidget.h
  inc/MantidQtWidgets/InstrumentView/MantidGLWidget.h
  inc/MantidQtWidgets/InstrumentView/OneCurvePlot.h
  inc/MantidQtWidgets/InstrumentView/CollapsiblePanel.h
  inc/MantidQtWidgets/InstrumentView/InstrumentActor.h
  inc/MantidQtWidgets/InstrumentView/PeakOverlay.h
  inc/MantidQtWidgets/InstrumentView/ProjectionSurface.h
  inc/MantidQtWidgets/InstrumentView/Projection3D.h
  inc/MantidQtWidgets/InstrumentView/Shape2DCollection.h
  inc/MantidQtWidgets/InstrumentView/UnwrappedSurface.h
  inc/MantidQtWidgets/InstrumentView/UCorrectionDialog.h
  inc/MantidQtWidgets/InstrumentView/XIntegrationControl.h
)

set ( INC_FILES
	inc/MantidQtWidgets/InstrumentView/BinDialog.h
	inc/MantidQtWidgets/InstrumentView/CollapsiblePanel.h
	inc/MantidQtWidgets/InstrumentView/ColorMapWidget.h
	inc/MantidQtWidgets/InstrumentView/CompAssemblyActor.h
	inc/MantidQtWidgets/InstrumentView/ComponentActor.h
	inc/MantidQtWidgets/InstrumentView/DetXMLFile.h
	inc/MantidQtWidgets/InstrumentView/DllOption.h
	inc/MantidQtWidgets/InstrumentView/GLActor.h
	inc/MantidQtWidgets/InstrumentView/GLActorCollection.h
	inc/MantidQtWidgets/InstrumentView/GLActorVisitor.h
	inc/MantidQtWidgets/InstrumentView/GLColor.h
	inc/MantidQtWidgets/InstrumentView/GLObject.h
	inc/MantidQtWidgets/InstrumentView/ICompAssemblyActor.h
	inc/MantidQtWidgets/InstrumentView/InstrumentActor.h
	inc/MantidQtWidgets/InstrumentView/InstrumentTreeModel.h
	inc/MantidQtWidgets/InstrumentView/InstrumentTreeWidget.h
	inc/MantidQtWidgets/InstrumentView/InstrumentWidget.h
	inc/MantidQtWidgets/InstrumentView/InstrumentWidgetMaskTab.h
	inc/MantidQtWidgets/InstrumentView/InstrumentWidgetPickTab.h
	inc/MantidQtWidgets/InstrumentView/InstrumentWidgetRenderTab.h
	inc/MantidQtWidgets/InstrumentView/InstrumentWidgetTab.h
	inc/MantidQtWidgets/InstrumentView/InstrumentWidgetTreeTab.h
	inc/MantidQtWidgets/InstrumentView/InstrumentWidgetTypes.h
	inc/MantidQtWidgets/InstrumentView/MantidGLWidget.h
	inc/MantidQtWidgets/InstrumentView/MaskBinsData.h
	inc/MantidQtWidgets/InstrumentView/ObjCompAssemblyActor.h
	inc/MantidQtWidgets/InstrumentView/ObjComponentActor.h
	inc/MantidQtWidgets/InstrumentView/OneCurvePlot.h
	inc/MantidQtWidgets/InstrumentView/OpenGLError.h
	inc/MantidQtWidgets/InstrumentView/PanelsSurface.h
	inc/MantidQtWidgets/InstrumentView/PeakMarker2D.h
	inc/MantidQtWidgets/InstrumentView/PeakOverlay.h
	inc/MantidQtWidgets/InstrumentView/Projection3D.h
	inc/MantidQtWidgets/InstrumentView/ProjectionSurface.h
	inc/MantidQtWidgets/InstrumentView/RectF.h
	inc/MantidQtWidgets/InstrumentView/RectangularDetectorActor.h
	inc/MantidQtWidgets/InstrumentView/RotationSurface.h
	inc/MantidQtWidgets/InstrumentView/SampleActor.h
	inc/MantidQtWidgets/InstrumentView/Shape2D.h
	inc/MantidQtWidgets/InstrumentView/Shape2DCollection.h
	inc/MantidQtWidgets/InstrumentView/SimpleWidget.h
	inc/MantidQtWidgets/InstrumentView/StructuredDetectorActor.h
	inc/MantidQtWidgets/InstrumentView/UCorrectionDialog.h
	inc/MantidQtWidgets/InstrumentView/UnwrappedCylinder.h
	inc/MantidQtWidgets/InstrumentView/UnwrappedSphere.h
	inc/MantidQtWidgets/InstrumentView/UnwrappedSurface.h
	inc/MantidQtWidgets/InstrumentView/Viewport.h
	inc/MantidQtWidgets/InstrumentView/XIntegrationControl.h
)

set ( UI_FILES
  inc/MantidQtWidgets/InstrumentView/UCorrectionDialog.ui
)

include_directories ( inc )

# Target
mtd_add_qt_library (TARGET_NAME MantidQtWidgetsInstrumentView
                    SRC ${SRC_FILES}
                    MOC ${MOC_FILES}
                    NOMOC ${INC_FILES}
                    UI ${UI_FILES}
                    DEFS IN_MANTIDQT_INSTRUMENTVIEW
                    LINK_LIBRARIES 
                      ${TCMALLOC_LIBRARIES_LINKTIME}
                      ${CORE_MANTIDLIBS}
                      ${QT_LIBRARIES}
                      ${QWT_LIBRARIES}
                      ${POCO_LIBRARIES}
                      ${Boost_LIBRARIES}
                      ${PYTHON_LIBRARIES}
                      ${OPENGL_LIBRARIES}
                    MTD_QT_LINK_LIBRARIES MantidQtWidgetsCommon
)

# qt4_wrap_cpp ( MOCCED_FILES ${MOC_FILES} )
# set ( ALL_SRC ${SRC_FILES} ${MOCCED_FILES} )
# qt4_wrap_ui ( UI_HDRS ${UI_FILES} )

# include_directories ( ${CMAKE_CURRENT_BINARY_DIR} )

# add_library ( MantidQtWidgetsInstrumentView ${ALL_SRC} ${INC_FILES} ${UI_HDRS} )

# set_target_properties ( MantidQtWidgetsInstrumentView PROPERTIES COMPILE_DEFINITIONS IN_MANTIDQT_INSTRUMENTVIEW )

if (OSX_VERSION VERSION_GREATER 10.8)
  set_target_properties ( MantidQtWidgetsInstrumentViewQt4 PROPERTIES INSTALL_RPATH "@loader_path/../MacOS;@loader_path/../Libraries")
endif ()

# install ( TARGETS MantidQtWidgetsInstrumentView ${SYSTEM_PACKAGE_TARGET} DESTINATION ${LIB_DIR} )
