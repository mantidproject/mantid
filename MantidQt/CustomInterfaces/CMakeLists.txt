set ( SRC_FILES
	src/DataComparison.cpp
	src/DirectConvertToEnergy.cpp
	src/DynamicPDF/DisplayCurveFitTest.cpp
	src/DynamicPDF/SliceSelector.cpp
        src/DynamicPDF/DPDFBackgroundRemover.cpp
	src/DynamicPDF/DPDFDisplayControl.cpp
	src/DynamicPDF/DPDFFitControl.cpp
	src/DynamicPDF/DPDFFitOptionsBrowser.cpp
	src/DynamicPDF/DPDFFourierTransform.cpp
	src/DynamicPDF/DPDFInputDataControl.cpp
	src/EnggDiffraction/EnggDiffractionPresenter.cpp
	src/EnggDiffraction/EnggDiffractionViewQtGUI.cpp
	src/Homer.cpp
	src/Indirect/AbsorptionCorrections.cpp
	src/Indirect/ApplyPaalmanPings.cpp
	src/Indirect/CalculatePaalmanPings.cpp
	src/Indirect/ContainerSubtraction.cpp
	src/Indirect/ConvFit.cpp
	src/Indirect/CorrectionsTab.cpp
	src/Indirect/DensityOfStates.cpp
	src/Indirect/Elwin.cpp
	src/Indirect/ILLCalibration.cpp
	src/Indirect/ILLEnergyTransfer.cpp
	src/Indirect/ISISCalibration.cpp
	src/Indirect/ISISDiagnostics.cpp
	src/Indirect/ISISEnergyTransfer.cpp
	src/Indirect/IndirectBayes.cpp
	src/Indirect/IndirectBayesTab.cpp
	src/Indirect/IndirectCorrections.cpp
	src/Indirect/IndirectDataAnalysis.cpp
	src/Indirect/IndirectDataAnalysisTab.cpp
	src/Indirect/IndirectDataReduction.cpp
	src/Indirect/IndirectDataReductionTab.cpp
	src/Indirect/IndirectDiffractionReduction.cpp
	src/Indirect/IndirectLoadILL.cpp
	src/Indirect/IndirectMolDyn.cpp
	src/Indirect/IndirectMoments.cpp
	src/Indirect/IndirectSassena.cpp
	src/Indirect/IndirectSimulation.cpp
	src/Indirect/IndirectSimulationTab.cpp
	src/Indirect/IndirectSqw.cpp
	src/Indirect/IndirectSymmetrise.cpp
	src/Indirect/IndirectTab.cpp
	src/Indirect/IndirectTools.cpp
	src/Indirect/IndirectToolsTab.cpp
	src/Indirect/IndirectTransmission.cpp
	src/Indirect/IndirectTransmissionCalc.cpp
	src/Indirect/Iqt.cpp
	src/Indirect/IqtFit.cpp
	src/Indirect/JumpFit.cpp
	src/Indirect/MSDFit.cpp
	src/Indirect/Quasi.cpp
	src/Indirect/ResNorm.cpp
	src/Indirect/Stretch.cpp
	src/MantidEV.cpp
	src/MantidEVWorker.cpp
	src/MultiDatasetFit/MDFAddWorkspaceDialog.cpp
	src/MultiDatasetFit/MDFDataController.cpp
	src/MultiDatasetFit/MDFDatasetPlotData.cpp
	src/MultiDatasetFit/MDFEditLocalParameterDialog.cpp
	src/MultiDatasetFit/MDFFunctionPlotData.cpp
	src/MultiDatasetFit/MDFLocalParameterEditor.cpp
	src/MultiDatasetFit/MDFLocalParameterItemDelegate.cpp
	src/MultiDatasetFit/MDFPlotController.cpp
	src/MultiDatasetFit/MultiDatasetFit.cpp
	src/Muon/ALCBaselineModellingModel.cpp
	src/Muon/ALCBaselineModellingPresenter.cpp
	src/Muon/ALCBaselineModellingView.cpp
	src/Muon/ALCDataLoadingPresenter.cpp
	src/Muon/ALCDataLoadingView.cpp
	src/Muon/ALCHelper.cpp
	src/Muon/ALCInterface.cpp
	src/Muon/ALCLatestFileFinder.cpp
	src/Muon/ALCPeakFittingModel.cpp
	src/Muon/ALCPeakFittingPresenter.cpp
	src/Muon/ALCPeakFittingView.cpp
	src/Muon/IO_MuonGrouping.cpp
	src/Muon/MuonAnalysis.cpp
	src/Muon/MuonAnalysisFitDataTab.cpp
	src/Muon/MuonAnalysisHelper.cpp
	src/Muon/MuonAnalysisOptionTab.cpp
	src/Muon/MuonAnalysisResultTableTab.cpp
	src/ParseKeyValueString.cpp
	src/Reflectometry/MeasurementItem.cpp
	src/Reflectometry/QReflTableModel.cpp
	src/Reflectometry/QReflTableView.cpp
	src/Reflectometry/QtReflMainView.cpp
	src/Reflectometry/QtReflOptionsDialog.cpp
	src/Reflectometry/ReflCatalogSearcher.cpp
	src/Reflectometry/ReflGenerateNotebook.cpp
	src/Reflectometry/ReflLegacyTransferStrategy.cpp
	src/Reflectometry/ReflMainViewPresenter.cpp
	src/Reflectometry/ReflMeasureTransferStrategy.cpp
	src/Reflectometry/ReflNexusMeasurementItemSource.cpp
	src/Reflectometry/ReflSearchModel.cpp
	src/Reflectometry/ReflTableSchema.cpp
	src/Reflectometry/ReflTableViewPresenter.cpp
	src/Reflectometry/TransferResults.cpp
	src/SANSAddFiles.cpp
	src/SANSBackgroundCorrectionSettings.cpp
	src/SANSBackgroundCorrectionWidget.cpp
	src/SANSConstants.cpp
	src/SANSDiagnostics.cpp
	src/SANSEventSlicing.cpp
	src/SANSPlotSpecial.cpp
	src/SANSRunWindow.cpp
	src/SampleTransmission.cpp
	src/StepScan.cpp
	src/Tomography/EnergyBandsViewQtGUI.cpp
	src/Tomography/ImageROIPresenter.cpp
	src/Tomography/ImageROIViewQtWidget.cpp
	src/Tomography/ImageStackPreParams.cpp
	src/Tomography/SavuConfigDialog.cpp
	src/Tomography/StackOfImagesDirs.cpp
	src/Tomography/TomoPathsConfig.cpp
	src/Tomography/TomoReconPostprocSettings.cpp
	src/Tomography/TomoReconPreprocSettings.cpp
	src/Tomography/TomoSystemSettings.cpp
	src/Tomography/TomoSystemSettingsLocal.cpp
	src/Tomography/TomoSystemSettingsRemote.cpp
	src/Tomography/TomoToolConfigDialog.cpp
	src/Tomography/TomographyIfaceModel.cpp
	src/Tomography/TomographyIfacePresenter.cpp
	src/Tomography/TomographyIfaceViewQtGUI.cpp
	src/Tomography/ToolConfig.cpp
	src/UserInputValidator.cpp
	src/background.cpp
	src/deltaECalc.cpp
)

# Include files aren't required, but this makes them appear in Visual Studio
# IMPORTANT: Include files are required in the MOC_FILES set. Scroll down to find it.
set ( INC_FILES
	inc/MantidQtCustomInterfaces/Background.h
	inc/MantidQtCustomInterfaces/DataComparison.h
	inc/MantidQtCustomInterfaces/DirectConvertToEnergy.h

	inc/MantidQtCustomInterfaces/DllConfig.h
	inc/MantidQtCustomInterfaces/DynamicPDF/DisplayCurveFitTest.h
	inc/MantidQtCustomInterfaces/DynamicPDF/SliceSelector.h
        inc/MantidQtCustomInterfaces/DynamicPDF/DPDFBackgroundRemover.h
	inc/MantidQtCustomInterfaces/DynamicPDF/DPDFDisplayControl.h
	inc/MantidQtCustomInterfaces/DynamicPDF/DPDFFitControl.h
	inc/MantidQtCustomInterfaces/DynamicPDF/DPDFFitOptionsBrowser.h
	inc/MantidQtCustomInterfaces/DynamicPDF/DPDFFourierTransform.h
	inc/MantidQtCustomInterfaces/DynamicPDF/DPDFInputDataControl.h
	inc/MantidQtCustomInterfaces/EnggDiffraction/EnggDiffCalibSettings.h
	inc/MantidQtCustomInterfaces/EnggDiffraction/EnggDiffractionPresWorker.h
	inc/MantidQtCustomInterfaces/EnggDiffraction/EnggDiffractionPresenter.h
	inc/MantidQtCustomInterfaces/EnggDiffraction/EnggDiffractionViewQtGUI.h
	inc/MantidQtCustomInterfaces/EnggDiffraction/IEnggDiffractionPresenter.h
	inc/MantidQtCustomInterfaces/EnggDiffraction/IEnggDiffractionView.h
	inc/MantidQtCustomInterfaces/Homer.h
	inc/MantidQtCustomInterfaces/Indirect/AbsorptionCorrections.h
	inc/MantidQtCustomInterfaces/Indirect/ApplyPaalmanPings.h
	inc/MantidQtCustomInterfaces/Indirect/CalculatePaalmanPings.h
	inc/MantidQtCustomInterfaces/Indirect/ContainerSubtraction.h
	inc/MantidQtCustomInterfaces/Indirect/ConvFit.h
	inc/MantidQtCustomInterfaces/Indirect/CorrectionsTab.h
	inc/MantidQtCustomInterfaces/Indirect/DensityOfStates.h
	inc/MantidQtCustomInterfaces/Indirect/Elwin.h
	inc/MantidQtCustomInterfaces/Indirect/ILLCalibration.h
	inc/MantidQtCustomInterfaces/Indirect/ILLEnergyTransfer.h
	inc/MantidQtCustomInterfaces/Indirect/ISISCalibration.h
	inc/MantidQtCustomInterfaces/Indirect/ISISDiagnostics.h
	inc/MantidQtCustomInterfaces/Indirect/ISISEnergyTransfer.h
	inc/MantidQtCustomInterfaces/Indirect/IndirectBayes.h
	inc/MantidQtCustomInterfaces/Indirect/IndirectBayesTab.h
	inc/MantidQtCustomInterfaces/Indirect/IndirectCorrections.h
	inc/MantidQtCustomInterfaces/Indirect/IndirectDataAnalysis.h
	inc/MantidQtCustomInterfaces/Indirect/IndirectDataAnalysisTab.h
	inc/MantidQtCustomInterfaces/Indirect/IndirectDataReduction.h
	inc/MantidQtCustomInterfaces/Indirect/IndirectDataReductionTab.h
	inc/MantidQtCustomInterfaces/Indirect/IndirectDiffractionReduction.h
	inc/MantidQtCustomInterfaces/Indirect/IndirectLoadILL.h
	inc/MantidQtCustomInterfaces/Indirect/IndirectMolDyn.h
	inc/MantidQtCustomInterfaces/Indirect/IndirectMoments.h
	inc/MantidQtCustomInterfaces/Indirect/IndirectSassena.h
	inc/MantidQtCustomInterfaces/Indirect/IndirectSimulation.h
	inc/MantidQtCustomInterfaces/Indirect/IndirectSimulationTab.h
	inc/MantidQtCustomInterfaces/Indirect/IndirectSqw.h
	inc/MantidQtCustomInterfaces/Indirect/IndirectSymmetrise.h
	inc/MantidQtCustomInterfaces/Indirect/IndirectTab.h
	inc/MantidQtCustomInterfaces/Indirect/IndirectTools.h
	inc/MantidQtCustomInterfaces/Indirect/IndirectToolsTab.h
	inc/MantidQtCustomInterfaces/Indirect/IndirectTransmission.h
	inc/MantidQtCustomInterfaces/Indirect/IndirectTransmissionCalc.h
	inc/MantidQtCustomInterfaces/Indirect/Iqt.h
	inc/MantidQtCustomInterfaces/Indirect/IqtFit.h
	inc/MantidQtCustomInterfaces/Indirect/JumpFit.h
	inc/MantidQtCustomInterfaces/Indirect/MSDFit.h
	inc/MantidQtCustomInterfaces/Indirect/Quasi.h
	inc/MantidQtCustomInterfaces/Indirect/ResNorm.h
	inc/MantidQtCustomInterfaces/Indirect/Stretch.h
	inc/MantidQtCustomInterfaces/MantidEV.h
	inc/MantidQtCustomInterfaces/MantidEVWorker.h
	inc/MantidQtCustomInterfaces/MultiDatasetFit/MDFAddWorkspaceDialog.h
	inc/MantidQtCustomInterfaces/MultiDatasetFit/MDFDataController.h
	inc/MantidQtCustomInterfaces/MultiDatasetFit/MDFDatasetPlotData.h
	inc/MantidQtCustomInterfaces/MultiDatasetFit/MDFEditLocalParameterDialog.h
	inc/MantidQtCustomInterfaces/MultiDatasetFit/MDFFunctionPlotData.h
	inc/MantidQtCustomInterfaces/MultiDatasetFit/MDFLocalParameterEditor.h
	inc/MantidQtCustomInterfaces/MultiDatasetFit/MDFLocalParameterItemDelegate.h
	inc/MantidQtCustomInterfaces/MultiDatasetFit/MDFPlotController.h
	inc/MantidQtCustomInterfaces/MultiDatasetFit/MultiDatasetFit.h
	inc/MantidQtCustomInterfaces/Muon/ALCBaselineModellingModel.h
	inc/MantidQtCustomInterfaces/Muon/ALCBaselineModellingPresenter.h
	inc/MantidQtCustomInterfaces/Muon/ALCBaselineModellingView.h
	inc/MantidQtCustomInterfaces/Muon/ALCDataLoadingPresenter.h
	inc/MantidQtCustomInterfaces/Muon/ALCDataLoadingView.h
	inc/MantidQtCustomInterfaces/Muon/ALCHelper.h
	inc/MantidQtCustomInterfaces/Muon/ALCInterface.h
	inc/MantidQtCustomInterfaces/Muon/ALCLatestFileFinder.h
	inc/MantidQtCustomInterfaces/Muon/ALCPeakFittingModel.h
	inc/MantidQtCustomInterfaces/Muon/ALCPeakFittingPresenter.h
	inc/MantidQtCustomInterfaces/Muon/ALCPeakFittingView.h
	inc/MantidQtCustomInterfaces/Muon/IALCBaselineModellingModel.h
	inc/MantidQtCustomInterfaces/Muon/IALCBaselineModellingView.h
	inc/MantidQtCustomInterfaces/Muon/IALCDataLoadingView.h
	inc/MantidQtCustomInterfaces/Muon/IALCPeakFittingModel.h
	inc/MantidQtCustomInterfaces/Muon/IALCPeakFittingView.h
	inc/MantidQtCustomInterfaces/Muon/IO_MuonGrouping.h
	inc/MantidQtCustomInterfaces/Muon/MuonAnalysis.h
	inc/MantidQtCustomInterfaces/Muon/MuonAnalysisFitDataTab.h
	inc/MantidQtCustomInterfaces/Muon/MuonAnalysisHelper.h
	inc/MantidQtCustomInterfaces/Muon/MuonAnalysisOptionTab.h
	inc/MantidQtCustomInterfaces/Muon/MuonAnalysisResultTableTab.h
	inc/MantidQtCustomInterfaces/ParseKeyValueString.h
	inc/MantidQtCustomInterfaces/ProgressableView.h
	inc/MantidQtCustomInterfaces/Reflectometry/IReflPresenter.h
	inc/MantidQtCustomInterfaces/Reflectometry/IReflSearcher.h
	inc/MantidQtCustomInterfaces/Reflectometry/IReflTablePresenter.h
	inc/MantidQtCustomInterfaces/Reflectometry/MeasurementItem.h
	inc/MantidQtCustomInterfaces/Reflectometry/ProgressPresenter.h
	inc/MantidQtCustomInterfaces/Reflectometry/QReflTableModel.h
	inc/MantidQtCustomInterfaces/Reflectometry/QReflTableView.h
	inc/MantidQtCustomInterfaces/Reflectometry/QtReflMainView.h
	inc/MantidQtCustomInterfaces/Reflectometry/QtReflOptionsDialog.h
	inc/MantidQtCustomInterfaces/Reflectometry/ReflAppendRowCommand.h
	inc/MantidQtCustomInterfaces/Reflectometry/ReflCatalogSearcher.h
	inc/MantidQtCustomInterfaces/Reflectometry/ReflClearSelectedCommand.h
	inc/MantidQtCustomInterfaces/Reflectometry/ReflCommand.h
	inc/MantidQtCustomInterfaces/Reflectometry/ReflCommandAdapter.h
	inc/MantidQtCustomInterfaces/Reflectometry/ReflCopySelectedCommand.h
	inc/MantidQtCustomInterfaces/Reflectometry/ReflCutSelectedCommand.h
	inc/MantidQtCustomInterfaces/Reflectometry/ReflDeleteRowCommand.h
	inc/MantidQtCustomInterfaces/Reflectometry/ReflCommand.h
	inc/MantidQtCustomInterfaces/Reflectometry/ReflCommandBase.h
	inc/MantidQtCustomInterfaces/Reflectometry/ReflExpandCommand.h
	inc/MantidQtCustomInterfaces/Reflectometry/ReflGenerateNotebook.h
	inc/MantidQtCustomInterfaces/Reflectometry/ReflGroupRowsCommand.h
	inc/MantidQtCustomInterfaces/Reflectometry/ReflExportTableCommand.h
	inc/MantidQtCustomInterfaces/Reflectometry/ReflImportTableCommand.h
	inc/MantidQtCustomInterfaces/Reflectometry/ReflLegacyTransferStrategy.h
	inc/MantidQtCustomInterfaces/Reflectometry/ReflMainView.h
	inc/MantidQtCustomInterfaces/Reflectometry/ReflMainViewPresenter.h
	inc/MantidQtCustomInterfaces/Reflectometry/ReflMeasureTransferStrategy.h
	inc/MantidQtCustomInterfaces/Reflectometry/ReflMeasurementItemSource.h
	inc/MantidQtCustomInterfaces/Reflectometry/ReflNexusMeasurementItemSource.h
	inc/MantidQtCustomInterfaces/Reflectometry/ReflNewTableCommand.h
	inc/MantidQtCustomInterfaces/Reflectometry/ReflOptionsCommand.h
	inc/MantidQtCustomInterfaces/Reflectometry/ReflOpenTableCommand.h
	inc/MantidQtCustomInterfaces/Reflectometry/ReflPasteSelectedCommand.h
	inc/MantidQtCustomInterfaces/Reflectometry/ReflPlotGroupCommand.h
	inc/MantidQtCustomInterfaces/Reflectometry/ReflPlotRowCommand.h
	inc/MantidQtCustomInterfaces/Reflectometry/ReflPrependRowCommand.h
	inc/MantidQtCustomInterfaces/Reflectometry/ReflProcessCommand.h
	inc/MantidQtCustomInterfaces/Reflectometry/ReflSaveTableAsCommand.h
	inc/MantidQtCustomInterfaces/Reflectometry/ReflSaveTableCommand.h
	inc/MantidQtCustomInterfaces/Reflectometry/ReflSearchModel.h
	inc/MantidQtCustomInterfaces/Reflectometry/ReflTableSchema.h
	inc/MantidQtCustomInterfaces/Reflectometry/ReflTableView.h
	inc/MantidQtCustomInterfaces/Reflectometry/ReflTableViewPresenter.h
	inc/MantidQtCustomInterfaces/Reflectometry/ReflTransferStrategy.h
	inc/MantidQtCustomInterfaces/Reflectometry/ReflVectorString.h
	inc/MantidQtCustomInterfaces/Reflectometry/ReflWorkspaceCommand.h
	inc/MantidQtCustomInterfaces/Reflectometry/TransferResults.h
	inc/MantidQtCustomInterfaces/Reflectometry/WorkspaceReceiver.h
	inc/MantidQtCustomInterfaces/SANSAddFiles.h
	inc/MantidQtCustomInterfaces/SANSBackgroundCorrectionSettings.h
	inc/MantidQtCustomInterfaces/SANSBackgroundCorrectionWidget.h
	inc/MantidQtCustomInterfaces/SANSConstants.h
	inc/MantidQtCustomInterfaces/SANSDiagnostics.h
	inc/MantidQtCustomInterfaces/SANSEventSlicing.h
	inc/MantidQtCustomInterfaces/SANSPlotSpecial.h
	inc/MantidQtCustomInterfaces/SANSRunWindow.h
	inc/MantidQtCustomInterfaces/SampleTransmission.h
	inc/MantidQtCustomInterfaces/StepScan.h
	inc/MantidQtCustomInterfaces/Tomography/IImageROIPresenter.h
	inc/MantidQtCustomInterfaces/Tomography/IImageROIView.h
	inc/MantidQtCustomInterfaces/Tomography/ITomographyIfacePresenter.h
	inc/MantidQtCustomInterfaces/Tomography/ITomographyIfaceView.h
	inc/MantidQtCustomInterfaces/Tomography/ImageROIPresenter.h
	inc/MantidQtCustomInterfaces/Tomography/ImageROIViewQtWidget.h
	inc/MantidQtCustomInterfaces/Tomography/ImageStackPreParams.h
	inc/MantidQtCustomInterfaces/Tomography/StackOfImagesDirs.h
	inc/MantidQtCustomInterfaces/Tomography/TomoPathsConfig.h
	inc/MantidQtCustomInterfaces/Tomography/TomoRecToolConfig.h
	inc/MantidQtCustomInterfaces/Tomography/TomoReconPostprocSettings.h
	inc/MantidQtCustomInterfaces/Tomography/TomoReconPreprocSettings.h
	inc/MantidQtCustomInterfaces/Tomography/TomoReconToolsUserSettings.h
	inc/MantidQtCustomInterfaces/Tomography/TomoSystemSettings.h
	inc/MantidQtCustomInterfaces/Tomography/TomoSystemSettingsLocal.h
	inc/MantidQtCustomInterfaces/Tomography/TomoSystemSettingsRemote.h
	inc/MantidQtCustomInterfaces/Tomography/TomoToolConfigDialog.h
	inc/MantidQtCustomInterfaces/Tomography/TomographyIfaceModel.h
	inc/MantidQtCustomInterfaces/Tomography/TomographyIfacePresenter.h
	inc/MantidQtCustomInterfaces/Tomography/TomographyIfaceViewQtGUI.h
	inc/MantidQtCustomInterfaces/Tomography/ToolConfigAstraToolbox.h
	inc/MantidQtCustomInterfaces/Tomography/ToolConfigCustom.h
	inc/MantidQtCustomInterfaces/Tomography/ToolConfigTomoPy.h
	inc/MantidQtCustomInterfaces/Updateable.h
	inc/MantidQtCustomInterfaces/UserInputValidator.h
	inc/MantidQtCustomInterfaces/deltaECalc.h
)

set ( SRC_UNITY_IGNORE_FILES )

set ( MOC_FILES inc/MantidQtCustomInterfaces/Background.h
                inc/MantidQtCustomInterfaces/DataComparison.h
                inc/MantidQtCustomInterfaces/DirectConvertToEnergy.h
                inc/MantidQtCustomInterfaces/DynamicPDF/DisplayCurveFitTest.h
                inc/MantidQtCustomInterfaces/DynamicPDF/DPDFBackgroundRemover.h
                inc/MantidQtCustomInterfaces/DynamicPDF/DPDFDisplayControl.h
                inc/MantidQtCustomInterfaces/DynamicPDF/DPDFFitControl.h
                inc/MantidQtCustomInterfaces/DynamicPDF/DPDFFitOptionsBrowser.h
                inc/MantidQtCustomInterfaces/DynamicPDF/DPDFFourierTransform.h
                inc/MantidQtCustomInterfaces/DynamicPDF/DPDFInputDataControl.h
                inc/MantidQtCustomInterfaces/DynamicPDF/SliceSelector.h
                inc/MantidQtCustomInterfaces/EnggDiffraction/EnggDiffractionPresenter.h
                inc/MantidQtCustomInterfaces/EnggDiffraction/EnggDiffractionPresWorker.h
                inc/MantidQtCustomInterfaces/EnggDiffraction/EnggDiffractionViewQtGUI.h
                inc/MantidQtCustomInterfaces/Homer.h
                inc/MantidQtCustomInterfaces/Indirect/AbsorptionCorrections.h
                inc/MantidQtCustomInterfaces/Indirect/ApplyPaalmanPings.h
                inc/MantidQtCustomInterfaces/Indirect/CalculatePaalmanPings.h
                inc/MantidQtCustomInterfaces/Indirect/ContainerSubtraction.h
                inc/MantidQtCustomInterfaces/Indirect/ConvFit.h
                inc/MantidQtCustomInterfaces/Indirect/CorrectionsTab.h
                inc/MantidQtCustomInterfaces/Indirect/DensityOfStates.h
                inc/MantidQtCustomInterfaces/Indirect/Elwin.h
                inc/MantidQtCustomInterfaces/Indirect/ILLCalibration.h
                inc/MantidQtCustomInterfaces/Indirect/ILLEnergyTransfer.h
                inc/MantidQtCustomInterfaces/Indirect/IndirectBayes.h
                inc/MantidQtCustomInterfaces/Indirect/IndirectBayesTab.h
                inc/MantidQtCustomInterfaces/Indirect/IndirectCorrections.h
                inc/MantidQtCustomInterfaces/Indirect/IndirectDataAnalysis.h
                inc/MantidQtCustomInterfaces/Indirect/IndirectDataAnalysisTab.h
                inc/MantidQtCustomInterfaces/Indirect/IndirectDataReduction.h
                inc/MantidQtCustomInterfaces/Indirect/IndirectDataReductionTab.h
                inc/MantidQtCustomInterfaces/Indirect/IndirectDiffractionReduction.h
                inc/MantidQtCustomInterfaces/Indirect/IndirectLoadILL.h
                inc/MantidQtCustomInterfaces/Indirect/IndirectMolDyn.h
                inc/MantidQtCustomInterfaces/Indirect/IndirectMoments.h
                inc/MantidQtCustomInterfaces/Indirect/IndirectSassena.h
                inc/MantidQtCustomInterfaces/Indirect/IndirectSimulation.h
                inc/MantidQtCustomInterfaces/Indirect/IndirectSimulationTab.h
                inc/MantidQtCustomInterfaces/Indirect/IndirectSqw.h
                inc/MantidQtCustomInterfaces/Indirect/IndirectSymmetrise.h
                inc/MantidQtCustomInterfaces/Indirect/IndirectTransmission.h
                inc/MantidQtCustomInterfaces/Indirect/IndirectTransmissionCalc.h
                inc/MantidQtCustomInterfaces/Indirect/IndirectTools.h
                inc/MantidQtCustomInterfaces/Indirect/IndirectToolsTab.h
                inc/MantidQtCustomInterfaces/Indirect/IndirectTab.h
                inc/MantidQtCustomInterfaces/Indirect/Iqt.h
                inc/MantidQtCustomInterfaces/Indirect/IqtFit.h
                inc/MantidQtCustomInterfaces/Indirect/JumpFit.h
                inc/MantidQtCustomInterfaces/Indirect/MSDFit.h
                inc/MantidQtCustomInterfaces/Indirect/Quasi.h
                inc/MantidQtCustomInterfaces/Indirect/ResNorm.h
                inc/MantidQtCustomInterfaces/Indirect/Stretch.h
                inc/MantidQtCustomInterfaces/Indirect/ISISCalibration.h
                inc/MantidQtCustomInterfaces/Indirect/ISISDiagnostics.h
                inc/MantidQtCustomInterfaces/Indirect/ISISEnergyTransfer.h
                inc/MantidQtCustomInterfaces/MantidEV.h
                inc/MantidQtCustomInterfaces/MultiDatasetFit/MultiDatasetFit.h
                inc/MantidQtCustomInterfaces/MultiDatasetFit/MDFAddWorkspaceDialog.h
                inc/MantidQtCustomInterfaces/MultiDatasetFit/MDFDataController.h
                inc/MantidQtCustomInterfaces/MultiDatasetFit/MDFPlotController.h
                inc/MantidQtCustomInterfaces/MultiDatasetFit/MDFLocalParameterEditor.h
                inc/MantidQtCustomInterfaces/MultiDatasetFit/MDFLocalParameterItemDelegate.h
                inc/MantidQtCustomInterfaces/MultiDatasetFit/MDFEditLocalParameterDialog.h
                inc/MantidQtCustomInterfaces/Muon/ALCBaselineModellingPresenter.h
                inc/MantidQtCustomInterfaces/Muon/ALCBaselineModellingView.h
                inc/MantidQtCustomInterfaces/Muon/ALCDataLoadingPresenter.h
                inc/MantidQtCustomInterfaces/Muon/ALCInterface.h
                inc/MantidQtCustomInterfaces/Muon/ALCPeakFittingPresenter.h
                inc/MantidQtCustomInterfaces/Muon/IALCBaselineModellingView.h
                inc/MantidQtCustomInterfaces/Muon/IALCBaselineModellingModel.h
                inc/MantidQtCustomInterfaces/Muon/IALCDataLoadingView.h
                inc/MantidQtCustomInterfaces/Muon/IALCPeakFittingView.h
                inc/MantidQtCustomInterfaces/Muon/IALCPeakFittingModel.h
                inc/MantidQtCustomInterfaces/Muon/MuonAnalysis.h
                inc/MantidQtCustomInterfaces/Muon/MuonAnalysisFitDataTab.h
                inc/MantidQtCustomInterfaces/Muon/MuonAnalysisHelper.h
                inc/MantidQtCustomInterfaces/Muon/MuonAnalysisOptionTab.h
                inc/MantidQtCustomInterfaces/Muon/MuonAnalysisResultTableTab.h
		inc/MantidQtCustomInterfaces/Reflectometry/ReflCommandAdapter.h
                inc/MantidQtCustomInterfaces/Reflectometry/ReflSearchModel.h
                inc/MantidQtCustomInterfaces/Reflectometry/QReflTableModel.h
                inc/MantidQtCustomInterfaces/Reflectometry/QReflTableView.h
                inc/MantidQtCustomInterfaces/Reflectometry/QtReflMainView.h
                inc/MantidQtCustomInterfaces/Reflectometry/QtReflOptionsDialog.h
                inc/MantidQtCustomInterfaces/SampleTransmission.h
                inc/MantidQtCustomInterfaces/SANSBackgroundCorrectionWidget.h
                inc/MantidQtCustomInterfaces/SANSAddFiles.h
                inc/MantidQtCustomInterfaces/SANSPlotSpecial.h
                inc/MantidQtCustomInterfaces/SANSRunWindow.h
                inc/MantidQtCustomInterfaces/SANSEventSlicing.h
                inc/MantidQtCustomInterfaces/SANSDiagnostics.h
                inc/MantidQtCustomInterfaces/StepScan.h
                inc/MantidQtCustomInterfaces/Tomography/ImageROIViewQtWidget.h
                inc/MantidQtCustomInterfaces/Tomography/ImageROIPresenter.h
                inc/MantidQtCustomInterfaces/Tomography/TomographyIfacePresenter.h
                inc/MantidQtCustomInterfaces/Tomography/TomographyIfaceViewQtGUI.h
                inc/MantidQtCustomInterfaces/Tomography/TomoToolConfigDialog.h
)

set ( UI_FILES inc/MantidQtCustomInterfaces/DataComparison.ui
               inc/MantidQtCustomInterfaces/DirectConvertToEnergy.ui
               inc/MantidQtCustomInterfaces/DynamicPDF/DisplayCurveFitTest.ui
               inc/MantidQtCustomInterfaces/DynamicPDF/DPDFBackgroundRemover.ui
               inc/MantidQtCustomInterfaces/DynamicPDF/DPDFFitControl.ui
               inc/MantidQtCustomInterfaces/DynamicPDF/DPDFFourierTransform.ui
               inc/MantidQtCustomInterfaces/DynamicPDF/SliceSelector.ui
               inc/MantidQtCustomInterfaces/EnggDiffraction/EnggDiffractionQtGUI.ui
               inc/MantidQtCustomInterfaces/EnggDiffraction/EnggDiffractionQtTabCalib.ui
               inc/MantidQtCustomInterfaces/EnggDiffraction/EnggDiffractionQtTabFocus.ui
               inc/MantidQtCustomInterfaces/EnggDiffraction/EnggDiffractionQtTabPreproc.ui
               inc/MantidQtCustomInterfaces/EnggDiffraction/EnggDiffractionQtTabFitting.ui
               inc/MantidQtCustomInterfaces/EnggDiffraction/EnggDiffractionQtTabSettings.ui
               inc/MantidQtCustomInterfaces/Indirect/AbsorptionCorrections.ui
               inc/MantidQtCustomInterfaces/Indirect/ApplyPaalmanPings.ui
               inc/MantidQtCustomInterfaces/Indirect/CalculatePaalmanPings.ui
               inc/MantidQtCustomInterfaces/Indirect/ContainerSubtraction.ui
               inc/MantidQtCustomInterfaces/Indirect/ConvFit.ui
               inc/MantidQtCustomInterfaces/Indirect/DensityOfStates.ui
               inc/MantidQtCustomInterfaces/Indirect/Elwin.ui
               inc/MantidQtCustomInterfaces/Indirect/ILLCalibration.ui
               inc/MantidQtCustomInterfaces/Indirect/ILLEnergyTransfer.ui
               inc/MantidQtCustomInterfaces/Indirect/IndirectBayes.ui
               inc/MantidQtCustomInterfaces/Indirect/IndirectCorrections.ui
               inc/MantidQtCustomInterfaces/Indirect/IndirectDataAnalysis.ui
               inc/MantidQtCustomInterfaces/Indirect/IndirectDataReduction.ui
               inc/MantidQtCustomInterfaces/Indirect/IndirectDiffractionReduction.ui
               inc/MantidQtCustomInterfaces/Indirect/IndirectLoadILL.ui
               inc/MantidQtCustomInterfaces/Indirect/IndirectMolDyn.ui
               inc/MantidQtCustomInterfaces/Indirect/IndirectMoments.ui
               inc/MantidQtCustomInterfaces/Indirect/IndirectSassena.ui
               inc/MantidQtCustomInterfaces/Indirect/IndirectSimulation.ui
               inc/MantidQtCustomInterfaces/Indirect/IndirectSqw.ui
               inc/MantidQtCustomInterfaces/Indirect/IndirectSymmetrise.ui
               inc/MantidQtCustomInterfaces/Indirect/IndirectTools.ui
               inc/MantidQtCustomInterfaces/Indirect/IndirectTransmission.ui
               inc/MantidQtCustomInterfaces/Indirect/IndirectTransmissionCalc.ui
               inc/MantidQtCustomInterfaces/Indirect/Iqt.ui
               inc/MantidQtCustomInterfaces/Indirect/IqtFit.ui
               inc/MantidQtCustomInterfaces/Indirect/JumpFit.ui
               inc/MantidQtCustomInterfaces/Indirect/MSDFit.ui
               inc/MantidQtCustomInterfaces/Indirect/Quasi.ui
               inc/MantidQtCustomInterfaces/Indirect/ResNorm.ui
               inc/MantidQtCustomInterfaces/Indirect/Stretch.ui
               inc/MantidQtCustomInterfaces/Indirect/ISISCalibration.ui
               inc/MantidQtCustomInterfaces/Indirect/ISISDiagnostics.ui
               inc/MantidQtCustomInterfaces/Indirect/ISISEnergyTransfer.ui
               inc/MantidQtCustomInterfaces/MantidEV.ui
               inc/MantidQtCustomInterfaces/MultiDatasetFit/MultiDatasetFit.ui
               inc/MantidQtCustomInterfaces/MultiDatasetFit/MDFAddWorkspaceDialog.ui
               inc/MantidQtCustomInterfaces/MultiDatasetFit/EditLocalParameterDialog.ui
               inc/MantidQtCustomInterfaces/Muon/ALCBaselineModellingView.ui
               inc/MantidQtCustomInterfaces/Muon/ALCDataLoadingView.ui
               inc/MantidQtCustomInterfaces/Muon/ALCInterface.ui
               inc/MantidQtCustomInterfaces/Muon/ALCPeakFittingView.ui
               inc/MantidQtCustomInterfaces/Muon/MuonAnalysis.ui
               inc/MantidQtCustomInterfaces/Reflectometry/QReflTableView.ui
               inc/MantidQtCustomInterfaces/Reflectometry/ReflMainWidget.ui
               inc/MantidQtCustomInterfaces/Reflectometry/ReflOptionsDialog.ui
               inc/MantidQtCustomInterfaces/Reflectometry/ReflWindow.ui
               inc/MantidQtCustomInterfaces/SampleTransmission.ui
               inc/MantidQtCustomInterfaces/SANSBackgroundCorrectionWidget.ui
               inc/MantidQtCustomInterfaces/SANSPlotSpecial.ui
               inc/MantidQtCustomInterfaces/SANSRunWindow.ui
               inc/MantidQtCustomInterfaces/SANSEventSlicing.ui
               inc/MantidQtCustomInterfaces/StepScan.ui
               inc/MantidQtCustomInterfaces/Tomography/ImgFormatsConversion.ui
               inc/MantidQtCustomInterfaces/Tomography/ImageSelectCoRAndRegions.ui
               inc/MantidQtCustomInterfaces/Tomography/TomographyIfaceQtGUI.ui
               inc/MantidQtCustomInterfaces/Tomography/TomographyIfaceQtTabEnergy.ui
               inc/MantidQtCustomInterfaces/Tomography/TomographyIfaceQtTabFiltersSettings.ui
               inc/MantidQtCustomInterfaces/Tomography/TomographyIfaceQtTabRun.ui
               inc/MantidQtCustomInterfaces/Tomography/TomographyIfaceQtTabSetup.ui
               inc/MantidQtCustomInterfaces/Tomography/TomographyIfaceQtTabVisualize.ui
               inc/MantidQtCustomInterfaces/Tomography/TomographyIfaceQtTabSystemSettings.ui
               inc/MantidQtCustomInterfaces/Tomography/TomoToolConfigAstra.ui
               inc/MantidQtCustomInterfaces/Tomography/TomoToolConfigCustom.ui
               inc/MantidQtCustomInterfaces/Tomography/TomoToolConfigSavu.ui
               inc/MantidQtCustomInterfaces/Tomography/TomoToolConfigTomoPy.ui
)

set ( TEST_FILES
	ALCBaselineModellingModelTest.h
	ALCBaselineModellingPresenterTest.h
	ALCDataLoadingPresenterTest.h
	ALCLatestFileFinderTest.h
	ALCPeakFittingModelTest.h
	ALCPeakFittingPresenterTest.h
	EnggDiffractionPresenterTest.h
	IO_MuonGroupingTest.h
	ImageROIPresenterTest.h
	MeasurementItemTest.h
	MuonAnalysisHelperTest.h
	ParseKeyValueStringTest.h
	ReflCommandsTest.h
	ReflGenerateNotebookTest.h
	ReflLegacyTransferStrategyTest.h
	ReflMainViewPresenterTest.h
	ReflMeasureTransferStrategyTest.h
	ReflNexusMeasurementItemSourceTest.h
	ReflTableViewPresenterTest.h
	StackOfImagesDirsTest.h
	TomographyIfacePresenterTest.h
	UserInputValidatorTest.h
)

include_directories ( inc )
include_directories ( ../../QtPropertyBrowser/src )

# Ugly hack to get around issue in Boost version 1.48.0
# in conjunction with Qt 4.7.4 or greater
set ( qt_version ${QT_VERSION_MAJOR}.${QT_VERSION_MINOR}.${QT_VERSION_PATCH} )
if ( ${Boost_VERSION} GREATER 104799 AND ${qt_version} VERSION_GREATER 4.7.3 )
	set( extra_options "-DBOOST_TT_HAS_OPERATOR_HPP_INCLUDED")
else ( ${Boost_VERSION} GREATER 104799 AND ${qt_version} VERSION_GREATER 4.7.3 )
	set ( extra_options "" )
endif ( ${Boost_VERSION} GREATER 104799 AND ${qt_version} VERSION_GREATER 4.7.3 )

qt4_wrap_cpp ( MOCCED_FILES ${MOC_FILES} OPTIONS ${extra_options} )

qt4_add_resources ( RES_FILES icons/CustomInterfacesIcons.qrc )

set ( ALL_SRC ${SRC_FILES} ${MOCCED_FILES} )#${RES_FILES})

qt4_wrap_ui ( UI_HDRS ${UI_FILES} )
include_directories ( ${CMAKE_CURRENT_BINARY_DIR} )

if(UNITY_BUILD)
  include(UnityBuild)
  enable_unity_build(CustomInterfaces SRC_FILES SRC_UNITY_IGNORE_FILES 10)
endif(UNITY_BUILD)

# Use a precompiled header where they are supported
enable_precompiled_headers( inc/MantidQtCustomInterfaces/PrecompiledHeader.h  ALL_SRC )
add_library ( CustomInterfaces ${ALL_SRC} ${INC_FILES} ${UI_HDRS} )

# Set the name of the generated library
set_target_properties ( CustomInterfaces PROPERTIES OUTPUT_NAME MantidQtCustomInterfaces
                                                    COMPILE_DEFINITIONS IN_MANTIDQT_CUSTOMINTERFACES )

if (OSX_VERSION VERSION_GREATER 10.8)
  set_target_properties ( CustomInterfaces PROPERTIES INSTALL_RPATH "@loader_path/../../../Contents/MacOS")
endif ()

find_Package(Threads)

target_link_libraries ( CustomInterfaces LINK_PRIVATE ${TCMALLOC_LIBRARIES_LINKTIME}
            MantidQtAPI
            MantidWidgets
            QtPropertyBrowser
            ${CORE_MANTIDLIBS}
            ${QT_LIBRARIES}
            ${QWT_LIBRARIES}
            ${POCO_LIBRARIES}
            ${Boost_LIBRARIES}
            ${JSONCPP_LIBRARIES}
            ${CMAKE_THREAD_LIBS_INIT})

###########################################################################
# Testing
###########################################################################

add_subdirectory ( test )

###########################################################################
# Installation settings
###########################################################################

install ( TARGETS CustomInterfaces ${SYSTEM_PACKAGE_TARGET} DESTINATION ${PLUGINS_DIR}/qtplugins/mantid )
