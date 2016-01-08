set ( SRC_FILES
    src/AlgorithmSelectorWidget.cpp
    src/CatalogHelper.cpp
    src/CatalogSearch.cpp
    src/CatalogSelector.cpp
    src/CheckboxHeader.cpp
    src/DataSelector.cpp
    src/DiagResults.cpp
    src/DoubleDialogEditor.cpp
    src/DoubleSpinBox.cpp
    src/ErrorCurve.cpp
    src/FilenameDialogEditor.cpp
    src/FindDialog.cpp
    src/FindReplaceDialog.cpp
    src/FitPropertyBrowser.cpp
    src/FormulaDialogEditor.cpp
    src/FitOptionsBrowser.cpp
    src/FunctionBrowser.cpp
    src/HintingLineEdit.cpp
    src/InstrumentSelector.cpp
    src/IndirectInstrumentConfig.cpp
    src/InputController.cpp
    src/MantidHelpWindow.cpp
    src/MWDiag.cpp
    src/MWRunFiles.cpp
    src/MessageDisplay.cpp
    src/MultifitSetupDialog.cpp
    src/MuonFitPropertyBrowser.cpp
    src/MuonSequentialFitDialog.cpp
    src/PeakPicker.cpp
    src/PeriodicTableWidget.cpp
    src/PreviewPlot.cpp
    src/ProcessingAlgoWidget.cpp
    src/PropertyHandler.cpp
    src/RangeSelector.cpp
    src/RenameParDialog.cpp
    src/SafeQwtPlot.cpp
    src/SaveWorkspaces.cpp
    src/ScriptEditor.cpp
    src/SelectFunctionDialog.cpp
    src/SelectWorkspacesDialog.cpp
    src/SequentialFitDialog.cpp
    src/SlicingAlgorithmDialog.cpp
    src/SlitCalculator.cpp
    src/StringDialogEditor.cpp
    src/StringEditorFactory.cpp
    src/UserFunctionDialog.cpp
    src/WorkspaceEditorFactory.cpp
    src/WorkspaceSelector.cpp
    src/pqHelpWindow.cxx
    src/pythonCalc.cpp
    src/LineEditWithClear.cpp
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
    src/InstrumentWindow.cpp
    src/InstrumentWindowMaskTab.cpp
    src/InstrumentWindowPickTab.cpp
    src/InstrumentWindowRenderTab.cpp
    src/InstrumentWindowTab.cpp
    src/InstrumentWindowTreeTab.cpp
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
    src/RectangularDetectorActor.cpp
    src/RectF.cpp
    src/RotationSurface.cpp
    src/SampleActor.cpp
    src/Shape2D.cpp
    src/Shape2DCollection.cpp
    src/SimpleWidget.cpp
    src/UCorrectionDialog.cpp
    src/UnwrappedCylinder.cpp
    src/UnwrappedSphere.cpp
    src/UnwrappedSurface.cpp
    src/Viewport.cpp
    src/XIntegrationControl.cpp
)

# Header files with Q_OBJECT that qmake will "moc"
set ( MOC_FILES 
    inc/MantidQtMantidWidgets/AlgorithmSelectorWidget.h
    inc/MantidQtMantidWidgets/CheckboxHeader.h
    inc/MantidQtMantidWidgets/DataSelector.h
    inc/MantidQtMantidWidgets/DiagResults.h
    inc/MantidQtMantidWidgets/DoubleDialogEditor.h
    inc/MantidQtMantidWidgets/DoubleSpinBox.h
    inc/MantidQtMantidWidgets/FilenameDialogEditor.h
    inc/MantidQtMantidWidgets/FormulaDialogEditor.h
    inc/MantidQtMantidWidgets/FindReplaceDialog.h
    inc/MantidQtMantidWidgets/FindDialog.h
    inc/MantidQtMantidWidgets/FitPropertyBrowser.h
    inc/MantidQtMantidWidgets/FitOptionsBrowser.h
    inc/MantidQtMantidWidgets/FunctionBrowser.h
    inc/MantidQtMantidWidgets/HintingLineEdit.h
    inc/MantidQtMantidWidgets/CatalogSearch.h
    inc/MantidQtMantidWidgets/CatalogSelector.h
    inc/MantidQtMantidWidgets/InstrumentSelector.h
    inc/MantidQtMantidWidgets/IndirectInstrumentConfig.h
    inc/MantidQtMantidWidgets/InputController.h
    inc/MantidQtMantidWidgets/MantidHelpWindow.h
    inc/MantidQtMantidWidgets/MessageDisplay.h
    inc/MantidQtMantidWidgets/MultifitSetupDialog.h
    inc/MantidQtMantidWidgets/MuonFitPropertyBrowser.h
    inc/MantidQtMantidWidgets/MuonSequentialFitDialog.h
    inc/MantidQtMantidWidgets/MWDiag.h
    inc/MantidQtMantidWidgets/MWRunFiles.h
    inc/MantidQtMantidWidgets/PeakPicker.h
    inc/MantidQtMantidWidgets/PeriodicTableWidget.h
    inc/MantidQtMantidWidgets/pqHelpWindow.h
    inc/MantidQtMantidWidgets/PreviewPlot.h
    inc/MantidQtMantidWidgets/PropertyHandler.h
    inc/MantidQtMantidWidgets/ProcessingAlgoWidget.h
    inc/MantidQtMantidWidgets/pythonCalc.h
    inc/MantidQtMantidWidgets/RangeSelector.h
    inc/MantidQtMantidWidgets/RenameParDialog.h
    inc/MantidQtMantidWidgets/SafeQwtPlot.h
    inc/MantidQtMantidWidgets/SaveWorkspaces.h
    inc/MantidQtMantidWidgets/ScriptEditor.h
    inc/MantidQtMantidWidgets/SelectFunctionDialog.h
    inc/MantidQtMantidWidgets/SelectWorkspacesDialog.h
    inc/MantidQtMantidWidgets/SequentialFitDialog.h
    inc/MantidQtMantidWidgets/SlicingAlgorithmDialog.h
    inc/MantidQtMantidWidgets/SlitCalculator.h
    inc/MantidQtMantidWidgets/StringDialogEditor.h
    inc/MantidQtMantidWidgets/StringEditorFactory.h
    inc/MantidQtMantidWidgets/UserFunctionDialog.h
    inc/MantidQtMantidWidgets/WorkspaceEditorFactory.h
    inc/MantidQtMantidWidgets/WorkspaceSelector.h
    inc/MantidQtMantidWidgets/LineEditWithClear.h
    inc/MantidQtMantidWidgets/XIntegrationControl.h
    inc/MantidQtMantidWidgets/BinDialog.h
    inc/MantidQtMantidWidgets/InstrumentTreeModel.h
    inc/MantidQtMantidWidgets/InstrumentTreeWidget.h
    inc/MantidQtMantidWidgets/InstrumentWindow.h
    inc/MantidQtMantidWidgets/InstrumentWindowTab.h
    inc/MantidQtMantidWidgets/InstrumentWindowRenderTab.h
    inc/MantidQtMantidWidgets/InstrumentWindowPickTab.h
    inc/MantidQtMantidWidgets/InstrumentWindowMaskTab.h
    inc/MantidQtMantidWidgets/InstrumentWindowTreeTab.h
    inc/MantidQtMantidWidgets/ColorMapWidget.h
    inc/MantidQtMantidWidgets/MantidGLWidget.h
    inc/MantidQtMantidWidgets/OneCurvePlot.h
    inc/MantidQtMantidWidgets/CollapsiblePanel.h
    inc/MantidQtMantidWidgets/InstrumentActor.h
    inc/MantidQtMantidWidgets/PeakOverlay.h
    inc/MantidQtMantidWidgets/ProjectionSurface.h
    inc/MantidQtMantidWidgets/Projection3D.h
    inc/MantidQtMantidWidgets/Shape2DCollection.h
    inc/MantidQtMantidWidgets/UnwrappedSurface.h
    inc/MantidQtMantidWidgets/UCorrectionDialog.h
)

# Add the include files are NOT already in MOC_FILES
set ( INC_FILES
    ${MOC_FILES}
    inc/MantidQtMantidWidgets/AlgorithmHintStrategy.h
    inc/MantidQtMantidWidgets/CatalogHelper.h
    inc/MantidQtMantidWidgets/ErrorCurve.h
    inc/MantidQtMantidWidgets/WidgetDllOption.h
    inc/MantidQtMantidWidgets/HintStrategy.h
    inc/MantidQtMantidWidgets/BinDialog.h
    inc/MantidQtMantidWidgets/CollapsiblePanel.h
    inc/MantidQtMantidWidgets/ColorMapWidget.h
    inc/MantidQtMantidWidgets/CompAssemblyActor.h
    inc/MantidQtMantidWidgets/ComponentActor.h
    inc/MantidQtMantidWidgets/DetXMLFile.h
    inc/MantidQtMantidWidgets/GLActor.h
    inc/MantidQtMantidWidgets/GLActorCollection.h
    inc/MantidQtMantidWidgets/GLActorVisitor.h
    inc/MantidQtMantidWidgets/GLColor.h
    inc/MantidQtMantidWidgets/GLObject.h
    inc/MantidQtMantidWidgets/ICompAssemblyActor.h
    inc/MantidQtMantidWidgets/InstrumentActor.h
    inc/MantidQtMantidWidgets/InstrumentTreeModel.h
    inc/MantidQtMantidWidgets/InstrumentTreeWidget.h
    inc/MantidQtMantidWidgets/InstrumentWindow.h
    inc/MantidQtMantidWidgets/InstrumentWindowMaskTab.h
    inc/MantidQtMantidWidgets/InstrumentWindowPickTab.h
    inc/MantidQtMantidWidgets/InstrumentWindowRenderTab.h
    inc/MantidQtMantidWidgets/InstrumentWindowTab.h
    inc/MantidQtMantidWidgets/InstrumentWindowTreeTab.h
    inc/MantidQtMantidWidgets/InstrumentWindowTypes.h
    inc/MantidQtMantidWidgets/MantidGLWidget.h
    inc/MantidQtMantidWidgets/MaskBinsData.h
    inc/MantidQtMantidWidgets/ObjCompAssemblyActor.h
    inc/MantidQtMantidWidgets/ObjComponentActor.h
    inc/MantidQtMantidWidgets/OneCurvePlot.h
    inc/MantidQtMantidWidgets/OpenGLError.h
    inc/MantidQtMantidWidgets/PanelsSurface.h
    inc/MantidQtMantidWidgets/PeakMarker2D.h
    inc/MantidQtMantidWidgets/PeakOverlay.h
    inc/MantidQtMantidWidgets/Projection3D.h
    inc/MantidQtMantidWidgets/ProjectionSurface.h
    inc/MantidQtMantidWidgets/RectangularDetectorActor.h
    inc/MantidQtMantidWidgets/RectF.h
    inc/MantidQtMantidWidgets/RotationSurface.h
    inc/MantidQtMantidWidgets/SampleActor.h
    inc/MantidQtMantidWidgets/Shape2D.h
    inc/MantidQtMantidWidgets/Shape2DCollection.h
    inc/MantidQtMantidWidgets/SimpleWidget.h
    inc/MantidQtMantidWidgets/UCorrectionDialog.h
    inc/MantidQtMantidWidgets/UnwrappedCylinder.h
    inc/MantidQtMantidWidgets/UnwrappedSphere.h
    inc/MantidQtMantidWidgets/UnwrappedSurface.h
    inc/MantidQtMantidWidgets/Viewport.h
    inc/MantidQtMantidWidgets/XIntegrationControl.h
)

# QtDesigner UI files to process
set ( UI_FILES 
    inc/MantidQtMantidWidgets/DataSelector.ui
    inc/MantidQtMantidWidgets/CatalogSearch.ui
    inc/MantidQtMantidWidgets/CatalogSelector.ui
    inc/MantidQtMantidWidgets/IndirectInstrumentConfig.ui
    inc/MantidQtMantidWidgets/MWDiag.ui
    inc/MantidQtMantidWidgets/MWRunFiles.ui
    inc/MantidQtMantidWidgets/MultifitSetupDialog.ui
    inc/MantidQtMantidWidgets/MuonSequentialFitDialog.ui
    inc/MantidQtMantidWidgets/ProcessingAlgoWidget.ui
    inc/MantidQtMantidWidgets/RenameParDialog.ui
    inc/MantidQtMantidWidgets/SelectFunctionDialog.ui
    inc/MantidQtMantidWidgets/SequentialFitDialog.ui
    inc/MantidQtMantidWidgets/SlicingAlgorithmDialog.ui
    inc/MantidQtMantidWidgets/SlitCalculator.ui
    inc/MantidQtMantidWidgets/UserFunctionDialog.ui
    inc/MantidQtMantidWidgets/PeriodicTableWidget.ui
    inc/MantidQtMantidWidgets/PreviewPlot.ui
    inc/MantidQtMantidWidgets/pqHelpWindow.ui
    inc/MantidQtMantidWidgets/UCorrectionDialog.ui
)

# Python unit tests
set ( TEST_PY_FILES
  test/MWRunFilesTest.py
)

set ( TEST_FILES
  AlgorithmHintStrategyTest.h
)

find_package (Qt4 REQUIRED QtHelp QtWebKit QtNetwork QUIET)
include(${QT_USE_FILE})

include_directories ( ../../QtPropertyBrowser/src )
include_directories ( ../../MantidQt/API/inc )
include_directories ( ../../MantidQt/MantidWidgets/inc )
include_directories ( ../../MantidQt/MantidWidgets/inc/MantidQtMantidWidgets )
include_directories ( ../../MantidQt/SliceViewer/inc )
include_directories ( ../../MantidQt/SpectrumViewer/inc )
include_directories ( ../../MantidQt/Factory/inc )
qt4_wrap_cpp ( MOCCED_FILES ${MOC_FILES} )

set ( ALL_SRC ${SRC_FILES} ${MOCCED_FILES} )

qt4_wrap_ui ( UI_HDRS ${UI_FILES} ) 

include_directories ( ${QSCINTILLA_INCLUDE_DIR} )
add_definitions ( -DQSCINTILLA_DLL )     # Will only have an effect on Windows (as is desired)


# For Windows:
add_definitions ( -DIN_MANTIDQT_MANTIDWIDGETS -DQT_QTPROPERTYBROWSER_IMPORT )
# Use a precompiled header where they are supported
enable_precompiled_headers( inc/MantidQtMantidWidgets/PrecompiledHeader.h ALL_SRC )
add_library ( MantidWidgets ${ALL_SRC} ${INC_FILES} ${UI_HDRS} )

if (OSX_VERSION VERSION_GREATER 10.8)
  set_target_properties ( MantidWidgets PROPERTIES INSTALL_RPATH "@loader_path/../MacOS")
endif ()

target_link_libraries ( MantidWidgets LINK_PRIVATE ${TCMALLOC_LIBRARIES_LINKTIME} 
            MantidQtAPI
            QtPropertyBrowser
            ${CORE_MANTIDLIBS}
            ${QT_LIBRARIES} ${QWT_LIBRARIES}
            ${QSCINTILLA_LIBRARIES}
            ${POCO_LIBRARIES}
            ${Boost_LIBRARIES}
            ${ZLIB_LIBRARIES}
            ${OPENGL_LIBRARIES}
            )

###########################################################################
# Testing
###########################################################################

add_subdirectory ( test )

###########################################################################
# Installation settings
###########################################################################

install ( TARGETS MantidWidgets ${SYSTEM_PACKAGE_TARGET} DESTINATION ${LIB_DIR} )
