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
	src/DataProcessorUI/AbstractDataProcessorTreeModel.cpp
	src/DataProcessorUI/DataProcessorGenerateNotebook.cpp
	src/DataProcessorUI/DataProcessorOneLevelTreeManager.cpp
	src/DataProcessorUI/DataProcessorPostprocessingAlgorithm.cpp
	src/DataProcessorUI/DataProcessorPreprocessingAlgorithm.cpp
	src/DataProcessorUI/DataProcessorPreprocessMap.cpp
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

set ( MOC_FILES
  inc/Common/AlgorithmDialog.h
  inc/Common/AlgorithmRunner.h
  inc/Common/AlgorithmPropertiesWidget.h
  inc/Common/BatchAlgorithmRunner.h
  inc/Common/BoolPropertyWidget.h
  inc/Common/FilePropertyWidget.h
  inc/Common/GenericDialog.h
  inc/Common/ListPropertyWidget.h
  inc/Common/ManageUserDirectories.h
  inc/Common/MantidDialog.h
  inc/Common/MantidHelpInterface.h
  inc/Common/MantidWidget.h
  inc/Common/MWRunFiles.h
  inc/Common/OptionsPropertyWidget.h
  inc/Common/PropertyWidget.h
  inc/Common/PythonRunner.h
  inc/Common/QScienceSpinBox.h
  inc/Common/QtSignalChannel.h
  inc/Common/ScriptRepositoryView.h
  inc/Common/RepoTreeView.h
  inc/Common/RepoModel.h
  inc/Common/SyncedCheckboxes.h
  inc/Common/TextPropertyWidget.h
  inc/Common/UserSubWindow.h
  inc/Common/VatesViewerInterface.h
  inc/Common/WorkspaceObserver.h
  inc/Common/SelectionNotificationService.h
  inc/Common/AlgorithmSelectorWidget.h
  inc/Common/CheckboxHeader.h
  inc/Common/ColorBarWidget.h
  inc/Common/DataProcessorUI/AbstractDataProcessorTreeModel.h
  inc/Common/DataProcessorUI/DataProcessorCommandAdapter.h
  inc/Common/DataProcessorUI/GenericDataProcessorPresenter.h
  inc/Common/DataProcessorUI/GenericDataProcessorPresenterThread.h
  inc/Common/DataProcessorUI/GenericDataProcessorPresenterRowReducerWorker.h
  inc/Common/DataProcessorUI/GenericDataProcessorPresenterGroupReducerWorker.h
  inc/Common/DataProcessorUI/QDataProcessorOneLevelTreeModel.h
  inc/Common/DataProcessorUI/QDataProcessorTwoLevelTreeModel.h
  inc/Common/DataProcessorUI/QDataProcessorWidget.h
  inc/Common/DataProcessorUI/QtDataProcessorOptionsDialog.h
  inc/Common/DataSelector.h
  inc/Common/DiagResults.h
  inc/Common/DisplayCurveFit.h
  inc/Common/DoubleDialogEditor.h
  inc/Common/DoubleSpinBox.h
  inc/Common/FilenameDialogEditor.h
  inc/Common/FormulaDialogEditor.h
  inc/Common/FindReplaceDialog.h
  inc/Common/FindDialog.h
  inc/Common/FitPropertyBrowser.h
  inc/Common/FitOptionsBrowser.h
  inc/Common/FunctionBrowser.h
  inc/Common/HintingLineEdit.h
  inc/Common/CatalogSearch.h
  inc/Common/CatalogSelector.h
  inc/Common/InstrumentSelector.h
  inc/Common/IndirectInstrumentConfig.h
  inc/Common/InputController.h
  inc/Common/MantidWSIndexDialog.h
  inc/Common/MantidTreeWidget.h
  inc/Common/MantidHelpWindow.h
  inc/Common/MessageDisplay.h
  inc/Common/MultifitSetupDialog.h
  inc/Common/MuonFitDataSelector.h
  inc/Common/MuonFitPropertyBrowser.h
  inc/Common/MuonFunctionBrowser.h
  inc/Common/MWDiag.h
  inc/Common/MWView.h
  inc/Common/PeakPicker.h
  inc/Common/PeriodicTableWidget.h
  inc/Common/pqHelpWindow.h
  inc/Common/PreviewPlot.h
  inc/Common/PropertyHandler.h
  inc/Common/ProcessingAlgoWidget.h
  inc/Common/pythonCalc.h
  inc/Common/RangeSelector.h
  inc/Common/RenameParDialog.h
  inc/Common/SafeQwtPlot.h
  inc/Common/SaveWorkspaces.h
  inc/Common/ScriptEditor.h
  inc/Common/SelectFunctionDialog.h
  inc/Common/SelectWorkspacesDialog.h
  inc/Common/SequentialFitDialog.h
  inc/Common/SlicingAlgorithmDialog.h
  inc/Common/SlitCalculator.h
  inc/Common/StringDialogEditor.h
  inc/Common/StringEditorFactory.h
  inc/Common/TrackedAction.h
  inc/Common/UserFunctionDialog.h
  inc/Common/WorkspaceEditorFactory.h
  inc/Common/WorkspaceSelector.h
  inc/Common/LineEditWithClear.h
  inc/Common/LogValueSelector.h
  inc/Common/WorkspacePresenter/QWorkspaceDockView.h
)

# Include files aren't required, but this makes them appear in Visual Studio
set ( INC_FILES
	${MOC_FILES}
	inc/Common/AlgorithmInputHistory.h
	inc/Common/AlgorithmRunner.h
	inc/Common/BatchAlgorithmRunner.h
	inc/Common/DllOption.h
	inc/Common/FileDialogHandler.h
	inc/Common/FlowLayout.h
	inc/Common/GraphOptions.h
	inc/Common/DistributionOptions.h
	inc/Common/HelpWindow.h
	inc/Common/InterfaceFactory.h
	inc/Common/InterfaceManager.h
	inc/Common/IProjectSerialisable.h
	inc/Common/MantidColorMap.h
	inc/Common/MantidDesktopServices.h
	inc/Common/MantidQwtIMDWorkspaceData.h
	inc/Common/MantidQwtWorkspaceData.h
	inc/Common/MantidAlgorithmMetatype.h
	inc/Common/MdConstants.h
	inc/Common/MdPlottingCmapsProvider.h
	inc/Common/MdSettings.h
	inc/Common/NonOrthogonal.h
	inc/Common/pixmaps.h
	inc/Common/PlotAxis.h
	inc/Common/PowerScaleEngine.h
	inc/Common/PropertyWidgetFactory.h
	inc/Common/PythonSystemHeader.h
	inc/Common/PythonThreading.h
	inc/Common/QScienceSpinBox.h
	inc/Common/QwtRasterDataMD.h
	inc/Common/QwtRasterDataMDNonOrthogonal.h
	inc/Common/QwtWorkspaceBinData.h
	inc/Common/QwtWorkspaceSpectrumData.h
	inc/Common/ScaleEngine.h
	inc/Common/ScriptRepositoryView.h
	inc/Common/SelectionNotificationService.h
	inc/Common/SignalBlocker.h
	inc/Common/SignalRange.h
	inc/Common/TSVSerialiser.h
	inc/Common/qwt_compat.h
	inc/Common/WidgetScrollbarDecorator.h
	inc/Common/WindowIcons.h
	inc/Common/WorkspaceIcons.h
	inc/Common/AlgorithmHintStrategy.h
	inc/Common/CatalogHelper.h
	inc/Common/DataProcessorUI/DataProcessorAppendGroupCommand.h
	inc/Common/DataProcessorUI/DataProcessorAppendRowCommand.h
	inc/Common/DataProcessorUI/DataProcessorClearSelectedCommand.h
	inc/Common/DataProcessorUI/DataProcessorCommand.h
	inc/Common/DataProcessorUI/DataProcessorCommandBase.h
  	inc/Common/DataProcessorUI/DataProcessorCollapseGroupsCommand.h
	inc/Common/DataProcessorUI/DataProcessorCopySelectedCommand.h
	inc/Common/DataProcessorUI/DataProcessorCutSelectedCommand.h
	inc/Common/DataProcessorUI/DataProcessorDeleteGroupCommand.h
	inc/Common/DataProcessorUI/DataProcessorDeleteRowCommand.h
	inc/Common/DataProcessorUI/DataProcessorExpandCommand.h
	inc/Common/DataProcessorUI/DataProcessorExpandGroupsCommand.h
	inc/Common/DataProcessorUI/DataProcessorExportTableCommand.h
	inc/Common/DataProcessorUI/DataProcessorGenerateNotebook.h
	inc/Common/DataProcessorUI/DataProcessorGroupRowsCommand.h
	inc/Common/DataProcessorUI/DataProcessorImportTableCommand.h
	inc/Common/DataProcessorUI/DataProcessorMainPresenter.h
	inc/Common/DataProcessorUI/DataProcessorNewTableCommand.h
	inc/Common/DataProcessorUI/DataProcessorOneLevelTreeManager.h
	inc/Common/DataProcessorUI/DataProcessorOpenTableCommand.h
	inc/Common/DataProcessorUI/DataProcessorOptionsCommand.h
	inc/Common/DataProcessorUI/DataProcessorPasteSelectedCommand.h
	inc/Common/DataProcessorUI/DataProcessorPauseCommand.h
	inc/Common/DataProcessorUI/DataProcessorPlotGroupCommand.h
	inc/Common/DataProcessorUI/DataProcessorPlotRowCommand.h
	inc/Common/DataProcessorUI/DataProcessorPostprocessingAlgorithm.h
	inc/Common/DataProcessorUI/DataProcessorPreprocessingAlgorithm.h
	inc/Common/DataProcessorUI/DataProcessorPreprocessMap.h
	inc/Common/DataProcessorUI/DataProcessorPresenter.h
	inc/Common/DataProcessorUI/DataProcessorProcessCommand.h
	inc/Common/DataProcessorUI/DataProcessorProcessingAlgorithm.h
	inc/Common/DataProcessorUI/DataProcessorProcessingAlgorithmBase.h
	inc/Common/DataProcessorUI/DataProcessorSaveTableAsCommand.h
	inc/Common/DataProcessorUI/DataProcessorSaveTableCommand.h
	inc/Common/DataProcessorUI/DataProcessorTreeManager.h
	inc/Common/DataProcessorUI/DataProcessorTwoLevelTreeManager.h
	inc/Common/DataProcessorUI/DataProcessorVectorString.h
	inc/Common/DataProcessorUI/DataProcessorView.h
	inc/Common/DataProcessorUI/DataProcessorWhiteList.h
	inc/Common/DataProcessorUI/DataProcessorWorkspaceCommand.h
	inc/Common/DataProcessorUI/GenericDataProcessorPresenterFactory.h
	inc/Common/DataProcessorUI/ParseKeyValueString.h
	inc/Common/ErrorCurve.h
	inc/Common/HintStrategy.h
	inc/Common/IFunctionBrowser.h
    inc/Common/IMuonFitDataModel.h
	inc/Common/IMuonFitDataSelector.h
	inc/Common/IMuonFitFunctionModel.h
	inc/Common/IProjectSaveView.h
	inc/Common/IWorkspaceFitControl.h
	inc/Common/LogValueSelector.h
	inc/Common/MantidDisplayBase.h
	inc/Common/MantidTreeWidgetItem.h
	inc/Common/MuonFitDataSelector.h
	inc/Common/MuonFunctionBrowser.h
    inc/Common/ProgressableView.h
    inc/Common/ProgressPresenter.h
    inc/Common/ProjectSavePresenter.h
    inc/Common/ProjectSaveModel.h
    inc/Common/ProgressableView.h
    inc/Common/WorkspacePresenter/ADSAdapter.h
    inc/Common/WorkspacePresenter/IWorkspaceDockView.h
    inc/Common/WorkspacePresenter/ViewNotifiable.h
    inc/Common/WorkspacePresenter/WorkspacePresenter.h
    inc/Common/WorkspacePresenter/WorkspaceProvider.h
    inc/Common/WorkspacePresenter/WorkspaceProviderNotifiable.h
    inc/Common/WorkspacePresenter/ADSAdapter.h
    inc/Common/WorkspacePresenter/IWorkspaceDockView.h
)

set ( UI_FILES
  inc/Common/ManageUserDirectories.ui
  inc/Common/MWRunFiles.ui
  inc/Common/ScriptRepositoryView.ui
  inc/Common/DataProcessorUI/DataProcessorOptionsDialog.ui
  inc/Common/DataProcessorUI/DataProcessorWidget.ui
  inc/Common/DataSelector.ui
  inc/Common/CatalogSearch.ui
  inc/Common/CatalogSelector.ui
  inc/Common/ColorBarWidget.ui
  inc/Common/DisplayCurveFit.ui
  inc/Common/IndirectInstrumentConfig.ui
  inc/Common/LogValueSelector.ui
  inc/Common/MWDiag.ui
  inc/Common/MWView.ui
  inc/Common/MultifitSetupDialog.ui
  inc/Common/MuonFitDataSelector.ui
  inc/Common/ProcessingAlgoWidget.ui
  inc/Common/RenameParDialog.ui
  inc/Common/SelectFunctionDialog.ui
  inc/Common/SequentialFitDialog.ui
  inc/Common/SlicingAlgorithmDialog.ui
  inc/Common/SlitCalculator.ui
  inc/Common/UserFunctionDialog.ui
  inc/Common/PeriodicTableWidget.ui
  inc/Common/PreviewPlot.ui
  inc/Common/pqHelpWindow.ui
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
   DataProcessorUI/DataProcessorCommandsTest.h
   DataProcessorUI/DataProcessorGenerateNotebookTest.h
   DataProcessorUI/DataProcessorOneLevelTreeManagerTest.h
   DataProcessorUI/DataProcessorPostprocessingAlgorithmTest.h
   DataProcessorUI/DataProcessorPreprocessingAlgorithmTest.h
   DataProcessorUI/DataProcessorPreprocessMapTest.h
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

qt4_wrap_cpp ( MOCCED_FILES ${MOC_FILES} )
set ( ALL_SRC ${SRC_FILES} ${MOCCED_FILES} )
qt4_wrap_ui ( UI_HDRS ${UI_FILES} )

if(MAKE_VATES AND ParaView_FOUND)
  include( ${PARAVIEW_USE_FILE} )
endif()

# Python includes
include_directories ( ${PYTHON_INCLUDE_PATH})
if(MAKE_VATES AND ParaView_FOUND)
  include_directories (SYSTEM ${PARAVIEW_INCLUDE_DIRS})
endif()
# Use a precompiled header where they are supported
enable_precompiled_headers( inc/Common/PrecompiledHeader.h ALL_SRC )
add_library ( qtwidgetscommon ${ALL_SRC} ${INC_FILES} ${UI_HDRS} )

set_target_properties ( qtwidgetscommon PROPERTIES COMPILE_DEFINITIONS IN_MANTIDQT_API )

if (OSX_VERSION VERSION_GREATER 10.8)
  set_target_properties ( qtwidgetscommon PROPERTIES INSTALL_RPATH "@loader_path/../MacOS;@loader_path/../Libraries")
endif ()

target_link_libraries ( qtwidgetscommon LINK_PRIVATE ${TCMALLOC_LIBRARIES_LINKTIME}
  ${CORE_MANTIDLIBS} ${QT_LIBRARIES} ${QWT_LIBRARIES}
  ${POCO_LIBRARIES} ${Boost_LIBRARIES} ${PYTHON_LIBRARIES}
)

if(MAKE_VATES AND ParaView_FOUND)
  target_link_libraries (qtwidgetscommon LINK_PRIVATE
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
  add_custom_command ( TARGET qtwidgetscommon POST_BUILD
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

install ( TARGETS qtwidgetscommon ${SYSTEM_PACKAGE_TARGET} DESTINATION ${LIB_DIR} )


set ( TEST_FILES
	ScriptRepositoryViewTest.h
)
