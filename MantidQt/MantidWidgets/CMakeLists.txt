set ( SRC_FILES
	src/AlgorithmSelectorWidget.cpp
	src/CatalogHelper.cpp
	src/CatalogSearch.cpp
	src/CatalogSelector.cpp
	src/CheckboxHeader.cpp
	src/ColorBarWidget.cpp
	src/DataProcessorUI/DataProcessorGenerateNotebook.cpp
	src/DataProcessorUI/DataProcessorOneLevelTreeManager.cpp
	src/DataProcessorUI/DataProcessorPostprocessingAlgorithm.cpp
	src/DataProcessorUI/DataProcessorPreprocessingAlgorithm.cpp
	src/DataProcessorUI/DataProcessorProcessingAlgorithm.cpp
	src/DataProcessorUI/DataProcessorProcessingAlgorithmBase.cpp
	src/DataProcessorUI/DataProcessorTwoLevelTreeManager.cpp
	src/DataProcessorUI/DataProcessorWhiteList.cpp
	src/DataProcessorUI/GenericDataProcessorPresenter.cpp
	src/DataProcessorUI/ParseKeyValueString.cpp
	src/DataProcessorUI/QDataProcessorOneLevelTreeModel.cpp
	src/DataProcessorUI/QDataProcessorTwoLevelTreeModel.cpp
	src/DataProcessorUI/QDataProcessorWidget.cpp
	src/DataProcessorUI/QtDataProcessorOptionsDialog.cpp
	src/DataSelector.cpp
	src/DiagResults.cpp
	src/DisplayCurveFit.cpp
	src/DoubleDialogEditor.cpp
	src/DoubleSpinBox.cpp
	src/ErrorCurve.cpp
	src/FilenameDialogEditor.cpp
	src/FindDialog.cpp
	src/FindReplaceDialog.cpp
	src/FitOptionsBrowser.cpp
	src/FitPropertyBrowser.cpp
	src/FormulaDialogEditor.cpp
	src/FunctionBrowser.cpp
	src/HintingLineEdit.cpp
	src/IndirectInstrumentConfig.cpp
	src/InputController.cpp
	src/InstrumentSelector.cpp
	src/InstrumentView/BinDialog.cpp
	src/InstrumentView/CollapsiblePanel.cpp
	src/InstrumentView/ColorMapWidget.cpp
	src/InstrumentView/CompAssemblyActor.cpp
	src/InstrumentView/ComponentActor.cpp
	src/InstrumentView/DetXMLFile.cpp
	src/InstrumentView/GLActor.cpp
	src/InstrumentView/GLActorCollection.cpp
	src/InstrumentView/GLActorVisitor.cpp
	src/InstrumentView/GLColor.cpp
	src/InstrumentView/GLObject.cpp
	src/InstrumentView/ICompAssemblyActor.cpp
	src/InstrumentView/InstrumentActor.cpp
	src/InstrumentView/InstrumentTreeModel.cpp
	src/InstrumentView/InstrumentTreeWidget.cpp
	src/InstrumentView/InstrumentWidget.cpp
	src/InstrumentView/InstrumentWidgetMaskTab.cpp
	src/InstrumentView/InstrumentWidgetPickTab.cpp
	src/InstrumentView/InstrumentWidgetRenderTab.cpp
	src/InstrumentView/InstrumentWidgetTab.cpp
	src/InstrumentView/InstrumentWidgetTreeTab.cpp
	src/InstrumentView/MantidGLWidget.cpp
	src/InstrumentView/MaskBinsData.cpp
	src/InstrumentView/ObjCompAssemblyActor.cpp
	src/InstrumentView/ObjComponentActor.cpp
	src/InstrumentView/OneCurvePlot.cpp
	src/InstrumentView/OpenGLError.cpp
	src/InstrumentView/PanelsSurface.cpp
	src/InstrumentView/PeakMarker2D.cpp
	src/InstrumentView/PeakOverlay.cpp
	src/InstrumentView/Projection3D.cpp
	src/InstrumentView/ProjectionSurface.cpp
	src/InstrumentView/RectF.cpp
	src/InstrumentView/RectangularDetectorActor.cpp
	src/InstrumentView/RotationSurface.cpp
	src/InstrumentView/SampleActor.cpp
	src/InstrumentView/Shape2D.cpp
	src/InstrumentView/Shape2DCollection.cpp
	src/InstrumentView/SimpleWidget.cpp
	src/InstrumentView/StructuredDetectorActor.cpp
	src/InstrumentView/UCorrectionDialog.cpp
	src/InstrumentView/UnwrappedCylinder.cpp
	src/InstrumentView/UnwrappedSphere.cpp
	src/InstrumentView/UnwrappedSurface.cpp
	src/InstrumentView/Viewport.cpp
	src/InstrumentView/XIntegrationControl.cpp
	src/LineEditWithClear.cpp
	src/LogValueSelector.cpp
	src/MWDiag.cpp
	src/MWView.cpp
	src/MantidHelpWindow.cpp
	src/MantidSurfacePlotDialog.cpp
	src/MantidTreeWidget.cpp
	src/MantidTreeWidgetItem.cpp
	src/MantidWSIndexDialog.cpp
	src/MessageDisplay.cpp
	src/MultifitSetupDialog.cpp
	src/MuonFitDataSelector.cpp
	src/MuonFitPropertyBrowser.cpp
	src/MuonFunctionBrowser.cpp
	src/PeakPicker.cpp
	src/PeriodicTableWidget.cpp
	src/PreviewPlot.cpp
	src/ProcessingAlgoWidget.cpp
        src/ProjectSavePresenter.cpp
        src/ProjectSaveModel.cpp
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
	src/TrackedAction.cpp
	src/UserFunctionDialog.cpp
	src/WorkspaceEditorFactory.cpp
	src/WorkspacePresenter/ADSAdapter.cpp
	src/WorkspacePresenter/QWorkspaceDockView.cpp
	src/WorkspacePresenter/WorkspacePresenter.cpp
	src/WorkspaceSelector.cpp
	src/pqHelpWindow.cxx
	src/pythonCalc.cpp
)

# Header files with Q_OBJECT that qmake will "moc"
set ( MOC_FILES
    inc/MantidQtMantidWidgets/AlgorithmSelectorWidget.h
    inc/MantidQtMantidWidgets/CheckboxHeader.h
    inc/MantidQtMantidWidgets/ColorBarWidget.h
    inc/MantidQtMantidWidgets/DataProcessorUI/DataProcessorCommandAdapter.h
    inc/MantidQtMantidWidgets/DataProcessorUI/QDataProcessorOneLevelTreeModel.h
    inc/MantidQtMantidWidgets/DataProcessorUI/QDataProcessorTwoLevelTreeModel.h
    inc/MantidQtMantidWidgets/DataProcessorUI/QDataProcessorWidget.h
    inc/MantidQtMantidWidgets/DataProcessorUI/QtDataProcessorOptionsDialog.h
    inc/MantidQtMantidWidgets/DataSelector.h
    inc/MantidQtMantidWidgets/DiagResults.h
    inc/MantidQtMantidWidgets/DisplayCurveFit.h
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
    inc/MantidQtMantidWidgets/MantidSurfacePlotDialog.h
    inc/MantidQtMantidWidgets/MantidWSIndexDialog.h
    inc/MantidQtMantidWidgets/MantidTreeWidget.h
    inc/MantidQtMantidWidgets/MantidHelpWindow.h
    inc/MantidQtMantidWidgets/MessageDisplay.h
    inc/MantidQtMantidWidgets/MultifitSetupDialog.h
    inc/MantidQtMantidWidgets/MuonFitDataSelector.h
    inc/MantidQtMantidWidgets/MuonFitPropertyBrowser.h
    inc/MantidQtMantidWidgets/MuonFunctionBrowser.h
    inc/MantidQtMantidWidgets/MWDiag.h
    inc/MantidQtMantidWidgets/MWView.h
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
    inc/MantidQtMantidWidgets/TrackedAction.h
    inc/MantidQtMantidWidgets/UserFunctionDialog.h
    inc/MantidQtMantidWidgets/WorkspaceEditorFactory.h
    inc/MantidQtMantidWidgets/WorkspaceSelector.h
    inc/MantidQtMantidWidgets/LineEditWithClear.h
    inc/MantidQtMantidWidgets/InstrumentView/BinDialog.h
    inc/MantidQtMantidWidgets/InstrumentView/InstrumentTreeModel.h
    inc/MantidQtMantidWidgets/InstrumentView/InstrumentTreeWidget.h
    inc/MantidQtMantidWidgets/InstrumentView/InstrumentWidget.h
    inc/MantidQtMantidWidgets/InstrumentView/InstrumentWidgetTab.h
    inc/MantidQtMantidWidgets/InstrumentView/InstrumentWidgetRenderTab.h
    inc/MantidQtMantidWidgets/InstrumentView/InstrumentWidgetPickTab.h
    inc/MantidQtMantidWidgets/InstrumentView/InstrumentWidgetMaskTab.h
    inc/MantidQtMantidWidgets/InstrumentView/InstrumentWidgetTreeTab.h
    inc/MantidQtMantidWidgets/InstrumentView/ColorMapWidget.h
    inc/MantidQtMantidWidgets/InstrumentView/MantidGLWidget.h
    inc/MantidQtMantidWidgets/InstrumentView/OneCurvePlot.h
    inc/MantidQtMantidWidgets/InstrumentView/CollapsiblePanel.h
    inc/MantidQtMantidWidgets/InstrumentView/InstrumentActor.h
    inc/MantidQtMantidWidgets/InstrumentView/PeakOverlay.h
    inc/MantidQtMantidWidgets/InstrumentView/ProjectionSurface.h
    inc/MantidQtMantidWidgets/InstrumentView/Projection3D.h
    inc/MantidQtMantidWidgets/InstrumentView/Shape2DCollection.h
    inc/MantidQtMantidWidgets/InstrumentView/UnwrappedSurface.h
    inc/MantidQtMantidWidgets/InstrumentView/UCorrectionDialog.h
    inc/MantidQtMantidWidgets/InstrumentView/XIntegrationControl.h
    inc/MantidQtMantidWidgets/LogValueSelector.h
    inc/MantidQtMantidWidgets/WorkspacePresenter/QWorkspaceDockView.h
)

# Add the include files are NOT already in MOC_FILES
set ( INC_FILES
	${MOC_FILES}
	inc/MantidQtMantidWidgets/AlgorithmHintStrategy.h
	inc/MantidQtMantidWidgets/CatalogHelper.h
	inc/MantidQtMantidWidgets/DataProcessorUI/DataProcessorAppendGroupCommand.h
	inc/MantidQtMantidWidgets/DataProcessorUI/DataProcessorAppendRowCommand.h
	inc/MantidQtMantidWidgets/DataProcessorUI/DataProcessorClearSelectedCommand.h
	inc/MantidQtMantidWidgets/DataProcessorUI/DataProcessorCommand.h
	inc/MantidQtMantidWidgets/DataProcessorUI/DataProcessorCommandBase.h
  inc/MantidQtMantidWidgets/DataProcessorUI/DataProcessorCollapseGroupsCommand.h
	inc/MantidQtMantidWidgets/DataProcessorUI/DataProcessorCopySelectedCommand.h
	inc/MantidQtMantidWidgets/DataProcessorUI/DataProcessorCutSelectedCommand.h
	inc/MantidQtMantidWidgets/DataProcessorUI/DataProcessorDeleteGroupCommand.h
	inc/MantidQtMantidWidgets/DataProcessorUI/DataProcessorDeleteRowCommand.h
	inc/MantidQtMantidWidgets/DataProcessorUI/DataProcessorExpandCommand.h
  inc/MantidQtMantidWidgets/DataProcessorUI/DataProcessorExpandGroupsCommand.h
	inc/MantidQtMantidWidgets/DataProcessorUI/DataProcessorExportTableCommand.h
	inc/MantidQtMantidWidgets/DataProcessorUI/DataProcessorGenerateNotebook.h
	inc/MantidQtMantidWidgets/DataProcessorUI/DataProcessorGroupRowsCommand.h
	inc/MantidQtMantidWidgets/DataProcessorUI/DataProcessorImportTableCommand.h
    inc/MantidQtMantidWidgets/DataProcessorUI/DataProcessorMainPresenter.h
	inc/MantidQtMantidWidgets/DataProcessorUI/DataProcessorNewTableCommand.h
	inc/MantidQtMantidWidgets/DataProcessorUI/DataProcessorOneLevelTreeManager.h
	inc/MantidQtMantidWidgets/DataProcessorUI/DataProcessorOpenTableCommand.h
	inc/MantidQtMantidWidgets/DataProcessorUI/DataProcessorOptionsCommand.h
	inc/MantidQtMantidWidgets/DataProcessorUI/DataProcessorPasteSelectedCommand.h
	inc/MantidQtMantidWidgets/DataProcessorUI/DataProcessorPlotGroupCommand.h
	inc/MantidQtMantidWidgets/DataProcessorUI/DataProcessorPlotRowCommand.h
	inc/MantidQtMantidWidgets/DataProcessorUI/DataProcessorPostprocessingAlgorithm.h
	inc/MantidQtMantidWidgets/DataProcessorUI/DataProcessorPreprocessingAlgorithm.h
	inc/MantidQtMantidWidgets/DataProcessorUI/DataProcessorPresenter.h
	inc/MantidQtMantidWidgets/DataProcessorUI/DataProcessorProcessCommand.h
	inc/MantidQtMantidWidgets/DataProcessorUI/DataProcessorProcessingAlgorithm.h
	inc/MantidQtMantidWidgets/DataProcessorUI/DataProcessorProcessingAlgorithmBase.h
	inc/MantidQtMantidWidgets/DataProcessorUI/DataProcessorSaveTableAsCommand.h
	inc/MantidQtMantidWidgets/DataProcessorUI/DataProcessorSaveTableCommand.h
	inc/MantidQtMantidWidgets/DataProcessorUI/DataProcessorTreeManager.h
	inc/MantidQtMantidWidgets/DataProcessorUI/DataProcessorTwoLevelTreeManager.h
	inc/MantidQtMantidWidgets/DataProcessorUI/DataProcessorVectorString.h
	inc/MantidQtMantidWidgets/DataProcessorUI/DataProcessorView.h
	inc/MantidQtMantidWidgets/DataProcessorUI/DataProcessorWhiteList.h
	inc/MantidQtMantidWidgets/DataProcessorUI/DataProcessorWorkspaceCommand.h
	inc/MantidQtMantidWidgets/DataProcessorUI/GenericDataProcessorPresenter.h
	inc/MantidQtMantidWidgets/DataProcessorUI/GenericDataProcessorPresenterFactory.h
	inc/MantidQtMantidWidgets/DataProcessorUI/ParseKeyValueString.h
	inc/MantidQtMantidWidgets/ErrorCurve.h
	inc/MantidQtMantidWidgets/HintStrategy.h
	inc/MantidQtMantidWidgets/IFunctionBrowser.h
    inc/MantidQtMantidWidgets/IMuonFitDataModel.h
	inc/MantidQtMantidWidgets/IMuonFitDataSelector.h
	inc/MantidQtMantidWidgets/IMuonFitFunctionModel.h
	inc/MantidQtMantidWidgets/IProjectSaveView.h
	inc/MantidQtMantidWidgets/IWorkspaceFitControl.h
	inc/MantidQtMantidWidgets/InstrumentView/BinDialog.h
	inc/MantidQtMantidWidgets/InstrumentView/CollapsiblePanel.h
	inc/MantidQtMantidWidgets/InstrumentView/ColorMapWidget.h
	inc/MantidQtMantidWidgets/InstrumentView/CompAssemblyActor.h
	inc/MantidQtMantidWidgets/InstrumentView/ComponentActor.h
	inc/MantidQtMantidWidgets/InstrumentView/DetXMLFile.h
	inc/MantidQtMantidWidgets/InstrumentView/GLActor.h
	inc/MantidQtMantidWidgets/InstrumentView/GLActorCollection.h
	inc/MantidQtMantidWidgets/InstrumentView/GLActorVisitor.h
	inc/MantidQtMantidWidgets/InstrumentView/GLColor.h
	inc/MantidQtMantidWidgets/InstrumentView/GLObject.h
	inc/MantidQtMantidWidgets/InstrumentView/ICompAssemblyActor.h
	inc/MantidQtMantidWidgets/InstrumentView/InstrumentActor.h
	inc/MantidQtMantidWidgets/InstrumentView/InstrumentTreeModel.h
	inc/MantidQtMantidWidgets/InstrumentView/InstrumentTreeWidget.h
	inc/MantidQtMantidWidgets/InstrumentView/InstrumentWidget.h
	inc/MantidQtMantidWidgets/InstrumentView/InstrumentWidgetMaskTab.h
	inc/MantidQtMantidWidgets/InstrumentView/InstrumentWidgetPickTab.h
	inc/MantidQtMantidWidgets/InstrumentView/InstrumentWidgetRenderTab.h
	inc/MantidQtMantidWidgets/InstrumentView/InstrumentWidgetTab.h
	inc/MantidQtMantidWidgets/InstrumentView/InstrumentWidgetTreeTab.h
	inc/MantidQtMantidWidgets/InstrumentView/InstrumentWidgetTypes.h
	inc/MantidQtMantidWidgets/InstrumentView/MantidGLWidget.h
	inc/MantidQtMantidWidgets/InstrumentView/MaskBinsData.h
	inc/MantidQtMantidWidgets/InstrumentView/ObjCompAssemblyActor.h
	inc/MantidQtMantidWidgets/InstrumentView/ObjComponentActor.h
	inc/MantidQtMantidWidgets/InstrumentView/OneCurvePlot.h
	inc/MantidQtMantidWidgets/InstrumentView/OpenGLError.h
	inc/MantidQtMantidWidgets/InstrumentView/PanelsSurface.h
	inc/MantidQtMantidWidgets/InstrumentView/PeakMarker2D.h
	inc/MantidQtMantidWidgets/InstrumentView/PeakOverlay.h
	inc/MantidQtMantidWidgets/InstrumentView/Projection3D.h
	inc/MantidQtMantidWidgets/InstrumentView/ProjectionSurface.h
	inc/MantidQtMantidWidgets/InstrumentView/RectF.h
	inc/MantidQtMantidWidgets/InstrumentView/RectangularDetectorActor.h
	inc/MantidQtMantidWidgets/InstrumentView/RotationSurface.h
	inc/MantidQtMantidWidgets/InstrumentView/SampleActor.h
	inc/MantidQtMantidWidgets/InstrumentView/Shape2D.h
	inc/MantidQtMantidWidgets/InstrumentView/Shape2DCollection.h
	inc/MantidQtMantidWidgets/InstrumentView/SimpleWidget.h
	inc/MantidQtMantidWidgets/InstrumentView/StructuredDetectorActor.h
	inc/MantidQtMantidWidgets/InstrumentView/UCorrectionDialog.h
	inc/MantidQtMantidWidgets/InstrumentView/UnwrappedCylinder.h
	inc/MantidQtMantidWidgets/InstrumentView/UnwrappedSphere.h
	inc/MantidQtMantidWidgets/InstrumentView/UnwrappedSurface.h
	inc/MantidQtMantidWidgets/InstrumentView/Viewport.h
	inc/MantidQtMantidWidgets/InstrumentView/XIntegrationControl.h
	inc/MantidQtMantidWidgets/LogValueSelector.h
	inc/MantidQtMantidWidgets/MantidDisplayBase.h
	inc/MantidQtMantidWidgets/MantidTreeWidgetItem.h
	inc/MantidQtMantidWidgets/MuonFitDataSelector.h
	inc/MantidQtMantidWidgets/MuonFunctionBrowser.h
    inc/MantidQtMantidWidgets/ProgressableView.h
    inc/MantidQtMantidWidgets/ProgressPresenter.h
    inc/MantidQtMantidWidgets/ProjectSavePresenter.h
    inc/MantidQtMantidWidgets/ProjectSaveModel.h
    inc/MantidQtMantidWidgets/ProgressableView.h
    inc/MantidQtMantidWidgets/WorkspacePresenter/ADSAdapter.h
    inc/MantidQtMantidWidgets/WorkspacePresenter/IWorkspaceDockView.h
    inc/MantidQtMantidWidgets/WorkspacePresenter/ViewNotifiable.h
    inc/MantidQtMantidWidgets/WorkspacePresenter/WorkspacePresenter.h
    inc/MantidQtMantidWidgets/WorkspacePresenter/WorkspaceProvider.h
    inc/MantidQtMantidWidgets/WorkspacePresenter/WorkspaceProviderNotifiable.h
    inc/MantidQtMantidWidgets/WorkspacePresenter/ADSAdapter.h
    inc/MantidQtMantidWidgets/WorkspacePresenter/IWorkspaceDockView.h
)

# QtDesigner UI files to process
set ( UI_FILES
    inc/MantidQtMantidWidgets/DataProcessorUI/DataProcessorOptionsDialog.ui
    inc/MantidQtMantidWidgets/DataProcessorUI/DataProcessorWidget.ui
    inc/MantidQtMantidWidgets/DataSelector.ui
    inc/MantidQtMantidWidgets/CatalogSearch.ui
    inc/MantidQtMantidWidgets/CatalogSelector.ui
    inc/MantidQtMantidWidgets/ColorBarWidget.ui
    inc/MantidQtMantidWidgets/DisplayCurveFit.ui
    inc/MantidQtMantidWidgets/IndirectInstrumentConfig.ui
    inc/MantidQtMantidWidgets/LogValueSelector.ui
    inc/MantidQtMantidWidgets/MWDiag.ui
    inc/MantidQtMantidWidgets/MWView.ui
    inc/MantidQtMantidWidgets/MultifitSetupDialog.ui
    inc/MantidQtMantidWidgets/MuonFitDataSelector.ui
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
    inc/MantidQtMantidWidgets/InstrumentView/UCorrectionDialog.ui
)

set ( TEST_FILES
        AlgorithmHintStrategyTest.h
        TrackedActionTest.h
        DataProcessorUI/DataProcessorCommandsTest.h
        DataProcessorUI/DataProcessorGenerateNotebookTest.h
        DataProcessorUI/DataProcessorOneLevelTreeManagerTest.h
        DataProcessorUI/DataProcessorPostprocessingAlgorithmTest.h
        DataProcessorUI/DataProcessorPreprocessingAlgorithmTest.h
        DataProcessorUI/DataProcessorProcessingAlgorithmBaseTest.h
        DataProcessorUI/DataProcessorProcessingAlgorithmTest.h
        DataProcessorUI/DataProcessorTwoLevelTreeManagerTest.h
        DataProcessorUI/DataProcessorWhiteListTest.h
        DataProcessorUI/GenericDataProcessorPresenterTest.h
        DataProcessorUI/ParseKeyValueStringTest.h
        DataProcessorUI/QDataProcessorOneLevelTreeModelTest.h
        DataProcessorUI/QDataProcessorTwoLevelTreeModelTest.h
        ProjectSaveModelTest.h
        ProjectSavePresenterTest.h
        WorkspacePresenter/ADSAdapterTest.h
        WorkspacePresenter/WorkspacePresenterTest.h
)

find_package (Qt4 REQUIRED QtHelp QtWebKit QtNetwork QUIET)
include(${QT_USE_FILE})

include_directories ( ../../QtPropertyBrowser/src )
include_directories ( ../../MantidQt/API/inc )
include_directories ( ../../MantidQt/MantidWidgets/inc )
include_directories ( ../../MantidQt/SliceViewer/inc )
include_directories ( ../../MantidQt/SpectrumViewer/inc )
include_directories ( ../../MantidQt/Factory/inc )
qt4_wrap_cpp ( MOCCED_FILES ${MOC_FILES} )

set ( ALL_SRC ${SRC_FILES} ${MOCCED_FILES} )

qt4_wrap_ui ( UI_HDRS ${UI_FILES} )
qt4_add_resources ( RES_FILES ${PROJECT_SOURCE_DIR}/images/MantidWidgets.qrc )

include_directories ( ${QSCINTILLA_INCLUDE_DIR} )
add_definitions ( -DQSCINTILLA_DLL )     # Will only have an effect on Windows (as is desired)


# For Windows:
add_definitions ( -DQT_QTPROPERTYBROWSER_IMPORT )
# Use a precompiled header where they are supported
enable_precompiled_headers( inc/MantidQtMantidWidgets/PrecompiledHeader.h ALL_SRC )
add_library ( MantidWidgets ${ALL_SRC} ${INC_FILES} ${UI_HDRS} ${RES_FILES} )

set_target_properties ( MantidWidgets PROPERTIES COMPILE_DEFINITIONS "IN_MANTIDQT_MANTIDWIDGETS")

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
