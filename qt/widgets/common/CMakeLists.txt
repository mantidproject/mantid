 set ( SRC_FILES
	src/AlgorithmDialog.cpp
	src/AlgorithmInputHistory.cpp
	src/AlgorithmPropertiesWidget.cpp
	src/AlgorithmRunner.cpp
	src/BatchAlgorithmRunner.cpp
	src/BoolPropertyWidget.cpp
	src/FileDialogHandler.cpp
	src/FilePropertyWidget.cpp
	src/GenericDialog.cpp
	src/HelpWindow.cpp
	src/FlowLayout.cpp
	src/InterfaceFactory.cpp
	src/InterfaceManager.cpp
	src/ListPropertyWidget.cpp
	src/ManageUserDirectories.cpp
	src/MantidColorMap.cpp
	src/MantidDesktopServices.cpp
	src/MantidDialog.cpp
	src/MantidHelpInterface.cpp
	src/MantidQwtIMDWorkspaceData.cpp
	src/MantidQwtWorkspaceData.cpp
	src/MantidWidget.cpp
	src/MdConstants.cpp
	src/MdPlottingCmapsProvider.cpp
	src/MdSettings.cpp
	src/Message.cpp
	src/NonOrthogonal.cpp
	src/MWRunFiles.cpp
	src/OptionsPropertyWidget.cpp
	src/pixmaps.cpp
	src/PlotAxis.cpp
	src/PowerScaleEngine.cpp
	src/PropertyWidget.cpp
	src/PropertyWidgetFactory.cpp
	src/PythonRunner.cpp
	src/PythonThreading.cpp
	src/QScienceSpinBox.cpp
	src/QtSignalChannel.cpp
        src/QwtHelper.cpp
	src/QwtRasterDataMD.cpp
	src/QwtRasterDataMDNonOrthogonal.cpp
	src/QwtWorkspaceBinData.cpp
	src/QwtWorkspaceSpectrumData.cpp
	src/RepoModel.cpp
	src/ScaleEngine.cpp
	src/ScriptRepositoryView.cpp
	src/SelectionNotificationService.cpp
	src/SignalBlocker.cpp
	src/SignalRange.cpp
	src/SyncedCheckboxes.cpp
	src/TextPropertyWidget.cpp
	src/TSVSerialiser.cpp
	src/UserSubWindow.cpp
	src/VatesViewerInterface.cpp
	src/WidgetScrollbarDecorator.cpp
    src/WindowIcons.cpp
	src/WorkspaceObserver.cpp
	src/WorkspaceIcons.cpp
	src/AlgorithmSelectorWidget.cpp
	src/CatalogHelper.cpp
	src/CatalogSearch.cpp
	src/CatalogSelector.cpp
	src/CheckboxHeader.cpp
	src/ColorBarWidget.cpp
	src/DataProcessorUI/AbstractTreeModel.cpp
	src/DataProcessorUI/GenerateNotebook.cpp
	src/DataProcessorUI/OneLevelTreeManager.cpp
	src/DataProcessorUI/PostprocessingAlgorithm.cpp
	src/DataProcessorUI/PreprocessingAlgorithm.cpp
	src/DataProcessorUI/PreprocessMap.cpp
	src/DataProcessorUI/ProcessingAlgorithm.cpp
	src/DataProcessorUI/ProcessingAlgorithmBase.cpp
	src/DataProcessorUI/TwoLevelTreeManager.cpp
	src/DataProcessorUI/WhiteList.cpp
    src/DataProcessorUI/PostprocessingStep.cpp
	src/DataProcessorUI/GenericDataProcessorPresenter.cpp
    src/DataProcessorUI/Column.cpp
    src/DataProcessorUI/ConstColumnIterator.cpp
	src/ParseKeyValueString.cpp
	src/ParseNumerics.cpp
	src/DataProcessorUI/QOneLevelTreeModel.cpp
	src/DataProcessorUI/QTwoLevelTreeModel.cpp
	src/DataProcessorUI/QDataProcessorWidget.cpp
	src/DataProcessorUI/QtDataProcessorOptionsDialog.cpp
	src/DataProcessorUI/VectorString.cpp
	src/DataSelector.cpp
	src/DiagResults.cpp
	src/DisplayCurveFit.cpp
	src/DoubleSpinBox.cpp
	src/ErrorCurve.cpp
	src/FindDialog.cpp
	src/FindReplaceDialog.cpp
	src/FitOptionsBrowser.cpp
	src/FitPropertyBrowser.cpp
	src/FunctionBrowser.cpp
	src/HintingLineEdit.cpp
	src/IndirectInstrumentConfig.cpp
	src/InputController.cpp
	src/InstrumentSelector.cpp
	src/LineEditWithClear.cpp
	src/LogValueSelector.cpp
	src/MWDiag.cpp
	src/MWView.cpp
	src/MantidHelpWindow.cpp
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
	src/TrackedAction.cpp
	src/UserFunctionDialog.cpp
	src/WorkspacePresenter/ADSAdapter.cpp
	src/WorkspacePresenter/QWorkspaceDockView.cpp
	src/WorkspacePresenter/WorkspacePresenter.cpp
	src/WorkspaceSelector.cpp
	src/pqHelpWindow.cxx
	src/pythonCalc.cpp

	src/QtPropertyBrowser/qtpropertybrowser.cpp
	src/QtPropertyBrowser/qtpropertymanager.cpp
	src/QtPropertyBrowser/qteditorfactory.cpp
	src/QtPropertyBrowser/qtvariantproperty.cpp
	src/QtPropertyBrowser/qttreepropertybrowser.cpp
	src/QtPropertyBrowser/qtbuttonpropertybrowser.cpp
	src/QtPropertyBrowser/qtgroupboxpropertybrowser.cpp
	src/QtPropertyBrowser/qtpropertybrowserutils.cpp
	src/QtPropertyBrowser/DoubleDialogEditor.cpp
	src/QtPropertyBrowser/DoubleEditorFactory.cpp
	src/QtPropertyBrowser/FilenameDialogEditor.cpp
    src/QtPropertyBrowser/FormulaDialogEditor.cpp
	src/QtPropertyBrowser/ParameterPropertyManager.cpp
	src/QtPropertyBrowser/StringDialogEditor.cpp
	src/QtPropertyBrowser/StringEditorFactory.cpp
	src/QtPropertyBrowser/WorkspaceEditorFactory.cpp
)

set ( MOC_FILES
  inc/MantidQtWidgets/Common/AlgorithmDialog.h
  inc/MantidQtWidgets/Common/AlgorithmRunner.h
  inc/MantidQtWidgets/Common/AlgorithmPropertiesWidget.h
  inc/MantidQtWidgets/Common/BatchAlgorithmRunner.h
  inc/MantidQtWidgets/Common/BoolPropertyWidget.h
  inc/MantidQtWidgets/Common/FilePropertyWidget.h
  inc/MantidQtWidgets/Common/GenericDialog.h
  inc/MantidQtWidgets/Common/ListPropertyWidget.h
  inc/MantidQtWidgets/Common/ManageUserDirectories.h
  inc/MantidQtWidgets/Common/MantidDialog.h
  inc/MantidQtWidgets/Common/MantidHelpInterface.h
  inc/MantidQtWidgets/Common/MantidWidget.h
  inc/MantidQtWidgets/Common/MWRunFiles.h
  inc/MantidQtWidgets/Common/OptionsPropertyWidget.h
  inc/MantidQtWidgets/Common/PropertyWidget.h
  inc/MantidQtWidgets/Common/PythonRunner.h
  inc/MantidQtWidgets/Common/QScienceSpinBox.h
  inc/MantidQtWidgets/Common/QtSignalChannel.h
  inc/MantidQtWidgets/Common/ScriptRepositoryView.h
  inc/MantidQtWidgets/Common/RepoTreeView.h
  inc/MantidQtWidgets/Common/RepoModel.h
  inc/MantidQtWidgets/Common/SyncedCheckboxes.h
  inc/MantidQtWidgets/Common/TextPropertyWidget.h
  inc/MantidQtWidgets/Common/UserSubWindow.h
  inc/MantidQtWidgets/Common/VatesViewerInterface.h
  inc/MantidQtWidgets/Common/WorkspaceObserver.h
  inc/MantidQtWidgets/Common/SelectionNotificationService.h
  inc/MantidQtWidgets/Common/AlgorithmSelectorWidget.h
  inc/MantidQtWidgets/Common/CheckboxHeader.h
  inc/MantidQtWidgets/Common/ColorBarWidget.h
  inc/MantidQtWidgets/Common/DataProcessorUI/AbstractTreeModel.h
  inc/MantidQtWidgets/Common/DataProcessorUI/QtCommandAdapter.h
  inc/MantidQtWidgets/Common/DataProcessorUI/GenericDataProcessorPresenter.h
  inc/MantidQtWidgets/Common/DataProcessorUI/GenericDataProcessorPresenterThread.h
  inc/MantidQtWidgets/Common/DataProcessorUI/GenericDataProcessorPresenterRowReducerWorker.h
  inc/MantidQtWidgets/Common/DataProcessorUI/GenericDataProcessorPresenterGroupReducerWorker.h
  inc/MantidQtWidgets/Common/DataProcessorUI/QOneLevelTreeModel.h
  inc/MantidQtWidgets/Common/DataProcessorUI/QTwoLevelTreeModel.h
  inc/MantidQtWidgets/Common/DataProcessorUI/QDataProcessorWidget.h
  inc/MantidQtWidgets/Common/DataProcessorUI/QtDataProcessorOptionsDialog.h
  inc/MantidQtWidgets/Common/DataProcessorUI/TreeData.h
  inc/MantidQtWidgets/Common/DataProcessorUI/ToStdStringMap.h
  inc/MantidQtWidgets/Common/DataSelector.h
  inc/MantidQtWidgets/Common/DiagResults.h
  inc/MantidQtWidgets/Common/DisplayCurveFit.h
  inc/MantidQtWidgets/Common/DoubleSpinBox.h
  inc/MantidQtWidgets/Common/FindReplaceDialog.h
  inc/MantidQtWidgets/Common/FindDialog.h
  inc/MantidQtWidgets/Common/FitPropertyBrowser.h
  inc/MantidQtWidgets/Common/FitOptionsBrowser.h
  inc/MantidQtWidgets/Common/FunctionBrowser.h
  inc/MantidQtWidgets/Common/HintingLineEdit.h
  inc/MantidQtWidgets/Common/CatalogSearch.h
  inc/MantidQtWidgets/Common/CatalogSelector.h
  inc/MantidQtWidgets/Common/InstrumentSelector.h
  inc/MantidQtWidgets/Common/IndirectInstrumentConfig.h
  inc/MantidQtWidgets/Common/InputController.h
  inc/MantidQtWidgets/Common/MantidWSIndexDialog.h
  inc/MantidQtWidgets/Common/MantidTreeWidget.h
  inc/MantidQtWidgets/Common/MantidHelpWindow.h
  inc/MantidQtWidgets/Common/MessageDisplay.h
  inc/MantidQtWidgets/Common/MultifitSetupDialog.h
  inc/MantidQtWidgets/Common/MuonFitDataSelector.h
  inc/MantidQtWidgets/Common/MuonFitPropertyBrowser.h
  inc/MantidQtWidgets/Common/MuonFunctionBrowser.h
  inc/MantidQtWidgets/Common/MWDiag.h
  inc/MantidQtWidgets/Common/MWView.h
  inc/MantidQtWidgets/Common/PeakPicker.h
  inc/MantidQtWidgets/Common/PeriodicTableWidget.h
  inc/MantidQtWidgets/Common/pqHelpWindow.h
  inc/MantidQtWidgets/Common/PreviewPlot.h
  inc/MantidQtWidgets/Common/PropertyHandler.h
  inc/MantidQtWidgets/Common/ProcessingAlgoWidget.h
  inc/MantidQtWidgets/Common/pythonCalc.h
  inc/MantidQtWidgets/Common/RangeSelector.h
  inc/MantidQtWidgets/Common/RenameParDialog.h
  inc/MantidQtWidgets/Common/SafeQwtPlot.h
  inc/MantidQtWidgets/Common/SaveWorkspaces.h
  inc/MantidQtWidgets/Common/ScriptEditor.h
  inc/MantidQtWidgets/Common/SelectFunctionDialog.h
  inc/MantidQtWidgets/Common/SelectWorkspacesDialog.h
  inc/MantidQtWidgets/Common/SequentialFitDialog.h
  inc/MantidQtWidgets/Common/SlicingAlgorithmDialog.h
  inc/MantidQtWidgets/Common/SlitCalculator.h
  inc/MantidQtWidgets/Common/TrackedAction.h
  inc/MantidQtWidgets/Common/UserFunctionDialog.h
  inc/MantidQtWidgets/Common/WorkspaceSelector.h
  inc/MantidQtWidgets/Common/LineEditWithClear.h
  inc/MantidQtWidgets/Common/LogValueSelector.h
  inc/MantidQtWidgets/Common/WorkspacePresenter/QWorkspaceDockView.h
  inc/MantidQtWidgets/Common/QtPropertyBrowser/ButtonEditorFactory.h
  inc/MantidQtWidgets/Common/QtPropertyBrowser/DoubleEditorFactory.h
  inc/MantidQtWidgets/Common/QtPropertyBrowser/DoubleDialogEditor.h
  inc/MantidQtWidgets/Common/QtPropertyBrowser/FormulaDialogEditor.h
  inc/MantidQtWidgets/Common/QtPropertyBrowser/FilenameDialogEditor.h
  inc/MantidQtWidgets/Common/QtPropertyBrowser/ParameterPropertyManager.h
  inc/MantidQtWidgets/Common/QtPropertyBrowser/StringDialogEditor.h
  inc/MantidQtWidgets/Common/QtPropertyBrowser/StringEditorFactory.h
  inc/MantidQtWidgets/Common/QtPropertyBrowser/WorkspaceEditorFactory.h
)

# Include files aren't required, but this makes them appear in Visual Studio
set ( INC_FILES
	${MOC_FILES}
	inc/MantidQtWidgets/Common/AlgorithmInputHistory.h
	inc/MantidQtWidgets/Common/AlgorithmRunner.h
	inc/MantidQtWidgets/Common/BatchAlgorithmRunner.h
	inc/MantidQtWidgets/Common/DllOption.h
	inc/MantidQtWidgets/Common/FileDialogHandler.h
	inc/MantidQtWidgets/Common/FlowLayout.h
	inc/MantidQtWidgets/Common/GraphOptions.h
	inc/MantidQtWidgets/Common/DistributionOptions.h
	inc/MantidQtWidgets/Common/HelpWindow.h
	inc/MantidQtWidgets/Common/InterfaceFactory.h
	inc/MantidQtWidgets/Common/InterfaceManager.h
	inc/MantidQtWidgets/Common/IProjectSerialisable.h
	inc/MantidQtWidgets/Common/MantidColorMap.h
	inc/MantidQtWidgets/Common/MantidDesktopServices.h
	inc/MantidQtWidgets/Common/MantidQwtIMDWorkspaceData.h
	inc/MantidQtWidgets/Common/MantidQwtWorkspaceData.h
	inc/MantidQtWidgets/Common/MantidAlgorithmMetatype.h
	inc/MantidQtWidgets/Common/MdConstants.h
	inc/MantidQtWidgets/Common/MdPlottingCmapsProvider.h
	inc/MantidQtWidgets/Common/MdSettings.h
	inc/MantidQtWidgets/Common/NonOrthogonal.h
	inc/MantidQtWidgets/Common/pixmaps.h
	inc/MantidQtWidgets/Common/PlotAxis.h
	inc/MantidQtWidgets/Common/PowerScaleEngine.h
	inc/MantidQtWidgets/Common/PropertyWidgetFactory.h
	inc/MantidQtWidgets/Common/PythonSystemHeader.h
	inc/MantidQtWidgets/Common/PythonThreading.h
	inc/MantidQtWidgets/Common/QScienceSpinBox.h
	inc/MantidQtWidgets/Common/QwtHelper.h
	inc/MantidQtWidgets/Common/QwtRasterDataMD.h
	inc/MantidQtWidgets/Common/QwtRasterDataMDNonOrthogonal.h
	inc/MantidQtWidgets/Common/QwtWorkspaceBinData.h
	inc/MantidQtWidgets/Common/QwtWorkspaceSpectrumData.h
	inc/MantidQtWidgets/Common/ScaleEngine.h
	inc/MantidQtWidgets/Common/ScriptRepositoryView.h
	inc/MantidQtWidgets/Common/SelectionNotificationService.h
	inc/MantidQtWidgets/Common/SignalBlocker.h
	inc/MantidQtWidgets/Common/SignalRange.h
	inc/MantidQtWidgets/Common/TSVSerialiser.h
	inc/MantidQtWidgets/Common/qwt_compat.h
	inc/MantidQtWidgets/Common/WidgetScrollbarDecorator.h
	inc/MantidQtWidgets/Common/WindowIcons.h
	inc/MantidQtWidgets/Common/WorkspaceIcons.h
	inc/MantidQtWidgets/Common/AlgorithmHintStrategy.h
	inc/MantidQtWidgets/Common/CatalogHelper.h
	inc/MantidQtWidgets/Common/DataProcessorUI/AppendGroupCommand.h
	inc/MantidQtWidgets/Common/DataProcessorUI/AppendRowCommand.h
	inc/MantidQtWidgets/Common/DataProcessorUI/ClearSelectedCommand.h
	inc/MantidQtWidgets/Common/DataProcessorUI/Command.h
	inc/MantidQtWidgets/Common/DataProcessorUI/CommandBase.h
  inc/MantidQtWidgets/Common/DataProcessorUI/CollapseGroupsCommand.h
	inc/MantidQtWidgets/Common/DataProcessorUI/CopySelectedCommand.h
	inc/MantidQtWidgets/Common/DataProcessorUI/CutSelectedCommand.h
	inc/MantidQtWidgets/Common/DataProcessorUI/DeleteGroupCommand.h
	inc/MantidQtWidgets/Common/DataProcessorUI/DeleteRowCommand.h
	inc/MantidQtWidgets/Common/DataProcessorUI/ExpandCommand.h
	inc/MantidQtWidgets/Common/DataProcessorUI/ExpandGroupsCommand.h
	inc/MantidQtWidgets/Common/DataProcessorUI/ExportTableCommand.h
	inc/MantidQtWidgets/Common/DataProcessorUI/GenerateNotebook.h
	inc/MantidQtWidgets/Common/DataProcessorUI/GroupRowsCommand.h
	inc/MantidQtWidgets/Common/DataProcessorUI/ImportTableCommand.h
	inc/MantidQtWidgets/Common/DataProcessorUI/DataProcessorMainPresenter.h
	inc/MantidQtWidgets/Common/DataProcessorUI/NewTableCommand.h
	inc/MantidQtWidgets/Common/DataProcessorUI/OneLevelTreeManager.h
	inc/MantidQtWidgets/Common/DataProcessorUI/OpenTableCommand.h
	inc/MantidQtWidgets/Common/DataProcessorUI/OptionsCommand.h
	inc/MantidQtWidgets/Common/DataProcessorUI/PasteSelectedCommand.h
	inc/MantidQtWidgets/Common/DataProcessorUI/PauseCommand.h
	inc/MantidQtWidgets/Common/DataProcessorUI/PlotGroupCommand.h
	inc/MantidQtWidgets/Common/DataProcessorUI/PlotRowCommand.h
	inc/MantidQtWidgets/Common/DataProcessorUI/PostprocessingAlgorithm.h
	inc/MantidQtWidgets/Common/DataProcessorUI/PreprocessingAlgorithm.h
	inc/MantidQtWidgets/Common/DataProcessorUI/PreprocessMap.h
	inc/MantidQtWidgets/Common/DataProcessorUI/DataProcessorPresenter.h
	inc/MantidQtWidgets/Common/DataProcessorUI/ProcessCommand.h
	inc/MantidQtWidgets/Common/DataProcessorUI/ProcessingAlgorithm.h
	inc/MantidQtWidgets/Common/DataProcessorUI/ProcessingAlgorithmBase.h
	inc/MantidQtWidgets/Common/DataProcessorUI/SaveTableAsCommand.h
	inc/MantidQtWidgets/Common/DataProcessorUI/SaveTableCommand.h
	inc/MantidQtWidgets/Common/DataProcessorUI/TreeManager.h
	inc/MantidQtWidgets/Common/DataProcessorUI/TwoLevelTreeManager.h
	inc/MantidQtWidgets/Common/DataProcessorUI/VectorString.h
	inc/MantidQtWidgets/Common/DataProcessorUI/DataProcessorView.h
	inc/MantidQtWidgets/Common/DataProcessorUI/WhiteList.h
	inc/MantidQtWidgets/Common/DataProcessorUI/WorkspaceCommand.h
	inc/MantidQtWidgets/Common/DataProcessorUI/GenericDataProcessorPresenterFactory.h
	inc/MantidQtWidgets/Common/ParseKeyValueString.h
	inc/MantidQtWidgets/Common/ParseNumerics.h
	inc/MantidQtWidgets/Common/ErrorCurve.h
	inc/MantidQtWidgets/Common/HintStrategy.h
	inc/MantidQtWidgets/Common/IFunctionBrowser.h
  inc/MantidQtWidgets/Common/IMuonFitDataModel.h
	inc/MantidQtWidgets/Common/IMuonFitDataSelector.h
	inc/MantidQtWidgets/Common/IMuonFitFunctionModel.h
	inc/MantidQtWidgets/Common/IProjectSaveView.h
	inc/MantidQtWidgets/Common/IWorkspaceFitControl.h
	inc/MantidQtWidgets/Common/LogValueSelector.h
	inc/MantidQtWidgets/Common/MantidDisplayBase.h
	inc/MantidQtWidgets/Common/MantidTreeWidgetItem.h
	inc/MantidQtWidgets/Common/MuonFitDataSelector.h
	inc/MantidQtWidgets/Common/MuonFunctionBrowser.h
  inc/MantidQtWidgets/Common/ProgressableView.h
  inc/MantidQtWidgets/Common/ProgressPresenter.h
  inc/MantidQtWidgets/Common/ProjectSavePresenter.h
  inc/MantidQtWidgets/Common/ProjectSaveModel.h
  inc/MantidQtWidgets/Common/ProgressableView.h
  inc/MantidQtWidgets/Common/WorkspacePresenter/ADSAdapter.h
  inc/MantidQtWidgets/Common/WorkspacePresenter/IWorkspaceDockView.h
  inc/MantidQtWidgets/Common/WorkspacePresenter/ViewNotifiable.h
  inc/MantidQtWidgets/Common/WorkspacePresenter/WorkspacePresenter.h
  inc/MantidQtWidgets/Common/WorkspacePresenter/WorkspaceProvider.h
  inc/MantidQtWidgets/Common/WorkspacePresenter/WorkspaceProviderNotifiable.h
  inc/MantidQtWidgets/Common/WorkspacePresenter/ADSAdapter.h
  inc/MantidQtWidgets/Common/WorkspacePresenter/IWorkspaceDockView.h
  inc/MantidQtWidgets/Common/QtPropertyBrowser/ButtonEditorFactory.h
  inc/MantidQtWidgets/Common/QtPropertyBrowser/CompositeEditorFactory.h
  inc/MantidQtWidgets/Common/QtPropertyBrowser/DoubleDialogEditor.h
  inc/MantidQtWidgets/Common/QtPropertyBrowser/DoubleEditorFactory.h
  inc/MantidQtWidgets/Common/QtPropertyBrowser/FilenameDialogEditor.h
  inc/MantidQtWidgets/Common/QtPropertyBrowser/FormulaDialogEditor.h
  inc/MantidQtWidgets/Common/QtPropertyBrowser/ParameterPropertyManager.h
  inc/MantidQtWidgets/Common/QtPropertyBrowser/StringDialogEditor.h
  inc/MantidQtWidgets/Common/QtPropertyBrowser/StringEditorFactory.h
  inc/MantidQtWidgets/Common/QtPropertyBrowser/WorkspaceEditorFactory.h
  inc/MantidQtWidgets/Common/QtPropertyBrowser/qtbuttonpropertybrowser.h
  inc/MantidQtWidgets/Common/QtPropertyBrowser/qteditorfactory.h
  inc/MantidQtWidgets/Common/QtPropertyBrowser/qtgroupboxpropertybrowser.h
  inc/MantidQtWidgets/Common/QtPropertyBrowser/qtpropertybrowser.h
  inc/MantidQtWidgets/Common/QtPropertyBrowser/qtpropertybrowserutils_p.h
  inc/MantidQtWidgets/Common/QtPropertyBrowser/qtpropertymanager.h
  inc/MantidQtWidgets/Common/QtPropertyBrowser/qttreepropertybrowser.h
  inc/MantidQtWidgets/Common/QtPropertyBrowser/qtvariantproperty.h

)

set ( UI_FILES
  inc/MantidQtWidgets/Common/ManageUserDirectories.ui
  inc/MantidQtWidgets/Common/MWRunFiles.ui
  inc/MantidQtWidgets/Common/ScriptRepositoryView.ui
  inc/MantidQtWidgets/Common/DataProcessorUI/DataProcessorOptionsDialog.ui
  inc/MantidQtWidgets/Common/DataProcessorUI/DataProcessorWidget.ui
  inc/MantidQtWidgets/Common/DataSelector.ui
  inc/MantidQtWidgets/Common/CatalogSearch.ui
  inc/MantidQtWidgets/Common/CatalogSelector.ui
  inc/MantidQtWidgets/Common/ColorBarWidget.ui
  inc/MantidQtWidgets/Common/DisplayCurveFit.ui
  inc/MantidQtWidgets/Common/IndirectInstrumentConfig.ui
  inc/MantidQtWidgets/Common/LogValueSelector.ui
  inc/MantidQtWidgets/Common/MWDiag.ui
  inc/MantidQtWidgets/Common/MWView.ui
  inc/MantidQtWidgets/Common/MultifitSetupDialog.ui
  inc/MantidQtWidgets/Common/MuonFitDataSelector.ui
  inc/MantidQtWidgets/Common/ProcessingAlgoWidget.ui
  inc/MantidQtWidgets/Common/RenameParDialog.ui
  inc/MantidQtWidgets/Common/SelectFunctionDialog.ui
  inc/MantidQtWidgets/Common/SequentialFitDialog.ui
  inc/MantidQtWidgets/Common/SlicingAlgorithmDialog.ui
  inc/MantidQtWidgets/Common/SlitCalculator.ui
  inc/MantidQtWidgets/Common/UserFunctionDialog.ui
  inc/MantidQtWidgets/Common/PeriodicTableWidget.ui
  inc/MantidQtWidgets/Common/PreviewPlot.ui
  inc/MantidQtWidgets/Common/pqHelpWindow.ui
)

# Python unit tests
set ( TEST_PY_FILES
  test/MWRunFilesTest.py
)

set( TEST_FILES
   BatchAlgorithmRunnerTest.h
   FileDialogHandlerTest.h
   InterfaceManagerTest.h
   MantidColorMapTest.h
   NonOrthogonalTest.h
   PlotAxisTest.h
   QwtWorkspaceBinDataTest.h
   QwtWorkspaceSpectrumDataTest.h
   SelectionNotificationServiceTest.h
   SignalRangeTest.h
   SignalBlockerTest.h
   AlgorithmHintStrategyTest.h
   TrackedActionTest.h
   DataProcessorUI/CommandsTest.h
   DataProcessorUI/GenerateNotebookTest.h
   DataProcessorUI/OneLevelTreeManagerTest.h
   DataProcessorUI/PostprocessingAlgorithmTest.h
   DataProcessorUI/PreprocessingAlgorithmTest.h
   DataProcessorUI/PreprocessMapTest.h
   DataProcessorUI/ProcessingAlgorithmBaseTest.h
   DataProcessorUI/ProcessingAlgorithmTest.h
   DataProcessorUI/TwoLevelTreeManagerTest.h
   DataProcessorUI/WhiteListTest.h
   DataProcessorUI/GenericDataProcessorPresenterTest.h
   ParseKeyValueStringTest.h
   DataProcessorUI/QOneLevelTreeModelTest.h
   DataProcessorUI/QTwoLevelTreeModelTest.h
   ProjectSaveModelTest.h
   ProjectSavePresenterTest.h
   WorkspacePresenter/ADSAdapterTest.h
   WorkspacePresenter/WorkspacePresenterTest.h
)
# moc'd files will end up in build directory, so add to include path
include_directories( ${CMAKE_CURRENT_BINARY_DIR} )

# Need to use a string of calls to the qt4_generate_moc macro
# instead of qt4_wrap_cpp because the latter produces files with
# a cxx suffix when QtPropertyBrowser requires cpp

qt4_generate_moc (
  inc/MantidQtWidgets/Common/QtPropertyBrowser/qtpropertybrowser.h
  ${CMAKE_CURRENT_BINARY_DIR}/moc_qtpropertybrowser.cpp
)
qt4_generate_moc (
  inc/MantidQtWidgets/Common/QtPropertyBrowser/qtpropertymanager.h
  ${CMAKE_CURRENT_BINARY_DIR}/moc_qtpropertymanager.cpp
)
qt4_generate_moc (
  inc/MantidQtWidgets/Common/QtPropertyBrowser/qteditorfactory.h
  ${CMAKE_CURRENT_BINARY_DIR}/moc_qteditorfactory.cpp
)
qt4_generate_moc (
  inc/MantidQtWidgets/Common/QtPropertyBrowser/qtvariantproperty.h
  ${CMAKE_CURRENT_BINARY_DIR}/moc_qtvariantproperty.cpp
)
qt4_generate_moc (
  inc/MantidQtWidgets/Common/QtPropertyBrowser/qttreepropertybrowser.h
  ${CMAKE_CURRENT_BINARY_DIR}/moc_qttreepropertybrowser.cpp
)
qt4_generate_moc (
  inc/MantidQtWidgets/Common/QtPropertyBrowser/qtbuttonpropertybrowser.h
  ${CMAKE_CURRENT_BINARY_DIR}/moc_qtbuttonpropertybrowser.cpp
)
qt4_generate_moc (
  inc/MantidQtWidgets/Common/QtPropertyBrowser/qtgroupboxpropertybrowser.h
  ${CMAKE_CURRENT_BINARY_DIR}/moc_qtgroupboxpropertybrowser.cpp
)
qt4_generate_moc (
  src/QtPropertyBrowser/qtpropertymanager.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/qtpropertymanager.moc
)
qt4_generate_moc (
  src/QtPropertyBrowser/qteditorfactory.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/qteditorfactory.moc
)
qt4_generate_moc (
  src/QtPropertyBrowser/qttreepropertybrowser.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/qttreepropertybrowser.moc
)

set (
  QTPROPERTYBROWSER_HEADERS_ONLY_MOC
  ${CMAKE_CURRENT_BINARY_DIR}/moc_qtpropertybrowser.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/moc_qtpropertymanager.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/moc_qteditorfactory.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/moc_qtvariantproperty.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/moc_qttreepropertybrowser.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/moc_qtbuttonpropertybrowser.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/moc_qtgroupboxpropertybrowser.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/qtpropertymanager.moc
  ${CMAKE_CURRENT_BINARY_DIR}/qteditorfactory.moc
  ${CMAKE_CURRENT_BINARY_DIR}/qttreepropertybrowser.moc
)

set_source_files_properties (
  ${QTPROPERTYBROWSER_HEADERS_ONLY_MOC}
  PROPERTIES
    HEADER_FILE_ONLY true
)

qt4_generate_moc (
  inc/MantidQtWidgets/Common/QtPropertyBrowser/qtpropertybrowserutils_p.h
  ${CMAKE_CURRENT_BINARY_DIR}/moc_qtpropertybrowserutils_p.cpp
)

set (
  QTPROPERTYBROWSER_HEADERS_MOC
  ${CMAKE_CURRENT_BINARY_DIR}/moc_qtpropertybrowserutils_p.cpp
)

set (
  QTPROPERTYBROWSER_MOC
  ${QTPROPERTYBROWSER_HEADERS_MOC}
  ${QTPROPERTYBROWSER_HEADERS_ONLY_MOC}
  ${EXTRA_MOCS}
)

find_package (Qt4 REQUIRED QtHelp QtWebKit QtNetwork QUIET)
include(${QT_USE_FILE})

qt4_wrap_cpp ( MOCCED_FILES ${MOC_FILES} )
set ( ALL_SRC
  ${SRC_FILES}
  ${MOCCED_FILES}
  ${QTPROPERTYBROWSER_MOC}
  ${QTPROPERTYBROWSER_SRCS}
  ${QTPROPERTYBROWSER_RESOURCES})
qt4_wrap_ui ( UI_HDRS ${UI_FILES} )

if(MAKE_VATES AND ParaView_FOUND)
  include( ${PARAVIEW_USE_FILE} )
endif()

include_directories ( ${QSCINTILLA_INCLUDE_DIR} )
add_definitions ( -DQSCINTILLA_DLL )     # Will only have an effect on Windows (as is desired)

# Python includes
include_directories ( ${PYTHON_INCLUDE_PATH})
if(MAKE_VATES AND ParaView_FOUND)
  include_directories (SYSTEM ${PARAVIEW_INCLUDE_DIRS})
endif()
# Use a precompiled header where they are supported
enable_precompiled_headers( inc/MantidQtWidgets/Common/PrecompiledHeader.h ALL_SRC )
add_library ( MantidQtWidgetsCommon ${ALL_SRC} ${INC_FILES} ${UI_HDRS} )

set_target_properties ( MantidQtWidgetsCommon PROPERTIES COMPILE_DEFINITIONS IN_MANTIDQT_COMMON )

if (OSX_VERSION VERSION_GREATER 10.8)
  set_target_properties ( MantidQtWidgetsCommon PROPERTIES INSTALL_RPATH "@loader_path/../MacOS;@loader_path/../Libraries")
endif ()

target_link_libraries ( MantidQtWidgetsCommon LINK_PRIVATE ${TCMALLOC_LIBRARIES_LINKTIME}
  ${CORE_MANTIDLIBS} ${QT_LIBRARIES} ${QWT_LIBRARIES}
  ${POCO_LIBRARIES} ${Boost_LIBRARIES} ${PYTHON_LIBRARIES} ${QSCINTILLA_LIBRARIES}
)

if(MAKE_VATES AND ParaView_FOUND)
  target_link_libraries (MantidQtWidgetsCommon LINK_PRIVATE
  vtkPVServerManagerRendering
  ${vtkjsoncpp_LIBRARIES}
  )
endif()

###########################################################################
# Windows: Generate a qt.conf file so that the dev build finds the qt
# plugins in third party. Done in two steps to get it into the build
# directory that is selected at build time.
# This file is NOT packaged. See installers/WinInstaller for the package
# version.
###########################################################################
if ( WIN32 )
  set ( QT_CONF qt.conf )
  configure_file ( qt.conf.template ${CMAKE_CURRENT_BINARY_DIR}/${QT_CONF} @ONLY )
  add_custom_command ( TARGET MantidQtWidgetsCommon POST_BUILD
                       COMMAND ${CMAKE_COMMAND} ARGS -E copy_if_different
                               ${CMAKE_CURRENT_BINARY_DIR}/${QT_CONF}
                               ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}/${QT_CONF} )
endif()

###########################################################################
# Testing
###########################################################################

add_subdirectory ( test )


###########################################################################
# Installation settings
###########################################################################

install ( TARGETS MantidQtWidgetsCommon ${SYSTEM_PACKAGE_TARGET} DESTINATION ${LIB_DIR} )


set ( TEST_FILES
	ScriptRepositoryViewTest.h
)
