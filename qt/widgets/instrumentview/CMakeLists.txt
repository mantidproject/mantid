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
  inc/InstrumentView/BinDialog.h
  inc/InstrumentView/InstrumentTreeModel.h
  inc/InstrumentView/InstrumentTreeWidget.h
  inc/InstrumentView/InstrumentWidget.h
  inc/InstrumentView/InstrumentWidgetTab.h
  inc/InstrumentView/InstrumentWidgetRenderTab.h
  inc/InstrumentView/InstrumentWidgetPickTab.h
  inc/InstrumentView/InstrumentWidgetMaskTab.h
  inc/InstrumentView/InstrumentWidgetTreeTab.h
  inc/InstrumentView/ColorMapWidget.h
  inc/InstrumentView/MantidGLWidget.h
  inc/InstrumentView/OneCurvePlot.h
  inc/InstrumentView/CollapsiblePanel.h
  inc/InstrumentView/InstrumentActor.h
  inc/InstrumentView/PeakOverlay.h
  inc/InstrumentView/ProjectionSurface.h
  inc/InstrumentView/Projection3D.h
  inc/InstrumentView/Shape2DCollection.h
  inc/InstrumentView/UnwrappedSurface.h
  inc/InstrumentView/UCorrectionDialog.h
  inc/InstrumentView/XIntegrationControl.h
)

set ( INC_FILES
	inc/InstrumentView/BinDialog.h
	inc/InstrumentView/CollapsiblePanel.h
	inc/InstrumentView/ColorMapWidget.h
	inc/InstrumentView/CompAssemblyActor.h
	inc/InstrumentView/ComponentActor.h
	inc/InstrumentView/DetXMLFile.h
	inc/InstrumentView/GLActor.h
	inc/InstrumentView/GLActorCollection.h
	inc/InstrumentView/GLActorVisitor.h
	inc/InstrumentView/GLColor.h
	inc/InstrumentView/GLObject.h
	inc/InstrumentView/ICompAssemblyActor.h
	inc/InstrumentView/InstrumentActor.h
	inc/InstrumentView/InstrumentTreeModel.h
	inc/InstrumentView/InstrumentTreeWidget.h
	inc/InstrumentView/InstrumentWidget.h
	inc/InstrumentView/InstrumentWidgetMaskTab.h
	inc/InstrumentView/InstrumentWidgetPickTab.h
	inc/InstrumentView/InstrumentWidgetRenderTab.h
	inc/InstrumentView/InstrumentWidgetTab.h
	inc/InstrumentView/InstrumentWidgetTreeTab.h
	inc/InstrumentView/InstrumentWidgetTypes.h
	inc/InstrumentView/MantidGLWidget.h
	inc/InstrumentView/MaskBinsData.h
	inc/InstrumentView/ObjCompAssemblyActor.h
	inc/InstrumentView/ObjComponentActor.h
	inc/InstrumentView/OneCurvePlot.h
	inc/InstrumentView/OpenGLError.h
	inc/InstrumentView/PanelsSurface.h
	inc/InstrumentView/PeakMarker2D.h
	inc/InstrumentView/PeakOverlay.h
	inc/InstrumentView/Projection3D.h
	inc/InstrumentView/ProjectionSurface.h
	inc/InstrumentView/RectF.h
	inc/InstrumentView/RectangularDetectorActor.h
	inc/InstrumentView/RotationSurface.h
	inc/InstrumentView/SampleActor.h
	inc/InstrumentView/Shape2D.h
	inc/InstrumentView/Shape2DCollection.h
	inc/InstrumentView/SimpleWidget.h
	inc/InstrumentView/StructuredDetectorActor.h
	inc/InstrumentView/UCorrectionDialog.h
	inc/InstrumentView/UnwrappedCylinder.h
	inc/InstrumentView/UnwrappedSphere.h
	inc/InstrumentView/UnwrappedSurface.h
	inc/InstrumentView/Viewport.h
	inc/InstrumentView/XIntegrationControl.h
)

set ( UI_FILES
  inc/InstrumentView/UCorrectionDialog.ui
)

qt4_wrap_cpp ( MOCCED_FILES ${MOC_FILES} )
set ( ALL_SRC ${SRC_FILES} ${MOCCED_FILES} )
qt4_wrap_ui ( UI_HDRS ${UI_FILES} )

add_library ( qtwidgetsinstrumentview ${ALL_SRC} ${INC_FILES} ${UI_HDRS} )

set_target_properties ( qtwidgetsinstrumentview PROPERTIES COMPILE_DEFINITIONS IN_MANTIDQT_API )

if (OSX_VERSION VERSION_GREATER 10.8)
  set_target_properties ( qtwidgetsinstrumentview PROPERTIES INSTALL_RPATH "@loader_path/../MacOS;@loader_path/../Libraries")
endif ()

target_link_libraries ( qtwidgetsinstrumentview LINK_PRIVATE ${TCMALLOC_LIBRARIES_LINKTIME}
  ${CORE_MANTIDLIBS} ${QT_LIBRARIES} ${QWT_LIBRARIES}
  ${POCO_LIBRARIES} ${Boost_LIBRARIES} ${PYTHON_LIBRARIES}
)

install ( TARGETS qtwidgetsinstrumentview ${SYSTEM_PACKAGE_TARGET} DESTINATION ${LIB_DIR} )
