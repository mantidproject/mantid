set ( SRC_FILES
	src/ApplyCorr.cpp
	src/CalcCorr.cpp
	src/ConvFit.cpp
  src/DensityOfStates.cpp
	src/DirectConvertToEnergy.cpp
	src/Elwin.cpp
	src/Fury.cpp
	src/FuryFit.cpp
	src/Homer.cpp
	src/IDATab.cpp
	src/IndirectBayes.cpp
	src/IndirectBayesTab.cpp
	src/IndirectCalibration.cpp
	src/IndirectConvertToEnergy.cpp
	src/IndirectDataAnalysis.cpp
	src/IndirectDataReduction.cpp
	src/IndirectDataReductionTab.cpp
	src/IndirectDiagnostics.cpp
	src/IndirectDiffractionReduction.cpp
  src/IndirectLoadILL.cpp
	src/IndirectMolDyn.cpp
	src/IndirectMoments.cpp
	src/IndirectSassena.cpp
	src/IndirectSimulation.cpp
	src/IndirectSimulationTab.cpp
  src/IndirectSqw.cpp
	src/IndirectSymmetrise.cpp
	src/IndirectTab.cpp
	src/IndirectTools.cpp
	src/IndirectToolsTab.cpp
  src/IndirectTransmission.cpp
  src/IndirectTransmissionCalc.cpp
	src/JumpFit.cpp
	src/MSDFit.cpp
	src/MantidEV.cpp
	src/MantidEVWorker.cpp
    src/MultiDatasetFit.cpp
	src/Muon/ALCBaselineModellingModel.cpp
	src/Muon/ALCBaselineModellingPresenter.cpp
	src/Muon/ALCBaselineModellingView.cpp
	src/Muon/ALCDataLoadingPresenter.cpp
	src/Muon/ALCDataLoadingView.cpp
	src/Muon/ALCHelper.cpp
	src/Muon/ALCInterface.cpp
	src/Muon/ALCPeakFittingModel.cpp
	src/Muon/ALCPeakFittingPresenter.cpp
	src/Muon/ALCPeakFittingView.cpp
	src/Muon/IO_MuonGrouping.cpp
	src/Muon/MuonAnalysis.cpp
	src/Muon/MuonAnalysisFitDataTab.cpp
	src/Muon/MuonAnalysisHelper.cpp
	src/Muon/MuonAnalysisOptionTab.cpp
	src/Muon/MuonAnalysisResultTableTab.cpp
	src/QReflTableModel.cpp
	src/QtReflMainView.cpp
	src/QtReflOptionsDialog.cpp
	src/Quasi.cpp
	src/ReflCatalogSearcher.cpp
	src/ReflLegacyTransferStrategy.cpp
	src/ReflMainViewPresenter.cpp
	src/ReflSearchModel.cpp
	src/ResNorm.cpp
	src/SANSAddFiles.cpp
	src/SANSDiagnostics.cpp
	src/SANSEventSlicing.cpp
	src/SANSPlotSpecial.cpp
	src/SANSRunWindow.cpp
	src/StepScan.cpp
	src/Stretch.cpp
	src/UserInputValidator.cpp
	src/background.cpp
	src/deltaECalc.cpp
)

# Include files aren't required, but this makes them appear in Visual Studio
set ( INC_FILES
	inc/MantidQtCustomInterfaces/ApplyCorr.h
	inc/MantidQtCustomInterfaces/Background.h
	inc/MantidQtCustomInterfaces/CalcCorr.h
	inc/MantidQtCustomInterfaces/ConvFit.h
  inc/MantidQtCustomInterfaces/DensityOfStates.h
	inc/MantidQtCustomInterfaces/DirectConvertToEnergy.h
	inc/MantidQtCustomInterfaces/DllConfig.h
	inc/MantidQtCustomInterfaces/Elwin.h
	inc/MantidQtCustomInterfaces/Fury.h
	inc/MantidQtCustomInterfaces/FuryFit.h
	inc/MantidQtCustomInterfaces/Homer.h
	inc/MantidQtCustomInterfaces/IDATab.h
	inc/MantidQtCustomInterfaces/IndirectBayes.h
	inc/MantidQtCustomInterfaces/IndirectBayesTab.h
	inc/MantidQtCustomInterfaces/IndirectCalibration.h
	inc/MantidQtCustomInterfaces/IndirectConvertToEnergy.h
	inc/MantidQtCustomInterfaces/IndirectDataAnalysis.h
	inc/MantidQtCustomInterfaces/IndirectDataReduction.h
 	inc/MantidQtCustomInterfaces/IndirectDataReductionTab.h
	inc/MantidQtCustomInterfaces/IndirectDiagnostics.h
	inc/MantidQtCustomInterfaces/IndirectDiffractionReduction.h
  inc/MantidQtCustomInterfaces/IndirectLoadILL.h
	inc/MantidQtCustomInterfaces/IndirectMolDyn.h
	inc/MantidQtCustomInterfaces/IndirectMoments.h
	inc/MantidQtCustomInterfaces/IndirectSassena.h
	inc/MantidQtCustomInterfaces/IndirectSimulation.h
	inc/MantidQtCustomInterfaces/IndirectSimulationTab.h
	inc/MantidQtCustomInterfaces/IndirectSqw.h
	inc/MantidQtCustomInterfaces/IndirectSymmetrise.h
  inc/MantidQtCustomInterfaces/IndirectTransmission.h
  inc/MantidQtCustomInterfaces/IndirectTransmissionCalc.h
	inc/MantidQtCustomInterfaces/IndirectTools.h
	inc/MantidQtCustomInterfaces/IndirectToolsTab.h
  inc/MantidQtCustomInterfaces/IndirectTab.h
	inc/MantidQtCustomInterfaces/IReflPresenter.h
	inc/MantidQtCustomInterfaces/IReflSearcher.h
	inc/MantidQtCustomInterfaces/JumpFit.h
	inc/MantidQtCustomInterfaces/MSDFit.h
	inc/MantidQtCustomInterfaces/MantidEV.h
	inc/MantidQtCustomInterfaces/MantidEVWorker.h
	inc/MantidQtCustomInterfaces/MultiDatasetFit.h
	inc/MantidQtCustomInterfaces/Muon/ALCBaselineModellingModel.h
	inc/MantidQtCustomInterfaces/Muon/ALCBaselineModellingPresenter.h
	inc/MantidQtCustomInterfaces/Muon/ALCBaselineModellingView.h
	inc/MantidQtCustomInterfaces/Muon/ALCDataLoadingPresenter.h
	inc/MantidQtCustomInterfaces/Muon/ALCDataLoadingView.h
	inc/MantidQtCustomInterfaces/Muon/ALCHelper.h
	inc/MantidQtCustomInterfaces/Muon/ALCInterface.h
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
	inc/MantidQtCustomInterfaces/ReflCatalogSearcher.h
	inc/MantidQtCustomInterfaces/ReflLegacyTransferStrategy.h
	inc/MantidQtCustomInterfaces/ReflMainView.h
	inc/MantidQtCustomInterfaces/ReflMainViewPresenter.h
	inc/MantidQtCustomInterfaces/ReflSearchModel.h
	inc/MantidQtCustomInterfaces/ReflTransferStrategy.h
	inc/MantidQtCustomInterfaces/QReflTableModel.h
	inc/MantidQtCustomInterfaces/QtReflMainView.h
	inc/MantidQtCustomInterfaces/QtReflOptionsDialog.h
	inc/MantidQtCustomInterfaces/Quasi.h
	inc/MantidQtCustomInterfaces/ResNorm.h
	inc/MantidQtCustomInterfaces/SANSAddFiles.h
	inc/MantidQtCustomInterfaces/SANSDiagnostics.h
	inc/MantidQtCustomInterfaces/SANSEventSlicing.h
	inc/MantidQtCustomInterfaces/SANSPlotSpecial.h
	inc/MantidQtCustomInterfaces/SANSRunWindow.h
	inc/MantidQtCustomInterfaces/StepScan.h
	inc/MantidQtCustomInterfaces/Stretch.h
	inc/MantidQtCustomInterfaces/Updateable.h
	inc/MantidQtCustomInterfaces/UserInputValidator.h
	inc/MantidQtCustomInterfaces/deltaECalc.h
)

set ( SRC_UNITY_IGNORE_FILES )

set ( MOC_FILES inc/MantidQtCustomInterfaces/Background.h
                inc/MantidQtCustomInterfaces/ApplyCorr.h
                inc/MantidQtCustomInterfaces/CalcCorr.h
                inc/MantidQtCustomInterfaces/ConvFit.h
                inc/MantidQtCustomInterfaces/DensityOfStates.h
                inc/MantidQtCustomInterfaces/DirectConvertToEnergy.h
                inc/MantidQtCustomInterfaces/Elwin.h
                inc/MantidQtCustomInterfaces/Fury.h
                inc/MantidQtCustomInterfaces/FuryFit.h
                inc/MantidQtCustomInterfaces/Homer.h
                inc/MantidQtCustomInterfaces/IDATab.h
                inc/MantidQtCustomInterfaces/IndirectBayes.h
                inc/MantidQtCustomInterfaces/IndirectBayesTab.h
                inc/MantidQtCustomInterfaces/IndirectCalibration.h
                inc/MantidQtCustomInterfaces/IndirectConvertToEnergy.h
                inc/MantidQtCustomInterfaces/IndirectDataAnalysis.h
                inc/MantidQtCustomInterfaces/IndirectDataReduction.h
                inc/MantidQtCustomInterfaces/IndirectDataReductionTab.h
                inc/MantidQtCustomInterfaces/IndirectDiagnostics.h
                inc/MantidQtCustomInterfaces/IndirectDiffractionReduction.h
                inc/MantidQtCustomInterfaces/IndirectLoadILL.h
                inc/MantidQtCustomInterfaces/IndirectMolDyn.h
                inc/MantidQtCustomInterfaces/IndirectMoments.h
                inc/MantidQtCustomInterfaces/IndirectSassena.h
                inc/MantidQtCustomInterfaces/IndirectSimulation.h
                inc/MantidQtCustomInterfaces/IndirectSimulationTab.h
                inc/MantidQtCustomInterfaces/IndirectSqw.h
                inc/MantidQtCustomInterfaces/IndirectSymmetrise.h
                inc/MantidQtCustomInterfaces/IndirectTransmission.h
                inc/MantidQtCustomInterfaces/IndirectTransmissionCalc.h
                inc/MantidQtCustomInterfaces/IndirectTools.h
                inc/MantidQtCustomInterfaces/IndirectToolsTab.h
                inc/MantidQtCustomInterfaces/IndirectTab.h
                inc/MantidQtCustomInterfaces/JumpFit.h
                inc/MantidQtCustomInterfaces/MSDFit.h
                inc/MantidQtCustomInterfaces/MultiDatasetFit.h
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
                inc/MantidQtCustomInterfaces/ReflSearchModel.h
                inc/MantidQtCustomInterfaces/QReflTableModel.h
                inc/MantidQtCustomInterfaces/QtReflMainView.h
                inc/MantidQtCustomInterfaces/QtReflOptionsDialog.h
                inc/MantidQtCustomInterfaces/Quasi.h
                inc/MantidQtCustomInterfaces/ResNorm.h
                inc/MantidQtCustomInterfaces/SANSAddFiles.h
                inc/MantidQtCustomInterfaces/SANSPlotSpecial.h
                inc/MantidQtCustomInterfaces/SANSRunWindow.h
                inc/MantidQtCustomInterfaces/SANSEventSlicing.h              
                inc/MantidQtCustomInterfaces/SANSDiagnostics.h
                inc/MantidQtCustomInterfaces/Stretch.h
                inc/MantidQtCustomInterfaces/MantidEV.h
                inc/MantidQtCustomInterfaces/StepScan.h
)

set ( UI_FILES inc/MantidQtCustomInterfaces/AddWorkspace.ui
               inc/MantidQtCustomInterfaces/DensityOfStates.ui
               inc/MantidQtCustomInterfaces/DirectConvertToEnergy.ui
               inc/MantidQtCustomInterfaces/IndirectBayes.ui
               inc/MantidQtCustomInterfaces/IndirectDataAnalysis.ui
               inc/MantidQtCustomInterfaces/IndirectDataReduction.ui
               inc/MantidQtCustomInterfaces/IndirectDiffractionReduction.ui
               inc/MantidQtCustomInterfaces/IndirectLoadILL.ui
               inc/MantidQtCustomInterfaces/IndirectMolDyn.ui
               inc/MantidQtCustomInterfaces/IndirectSassena.ui
               inc/MantidQtCustomInterfaces/IndirectSimulation.ui
               inc/MantidQtCustomInterfaces/IndirectTools.ui
               inc/MantidQtCustomInterfaces/IndirectTransmissionCalc.ui
               inc/MantidQtCustomInterfaces/JumpFit.ui
               inc/MantidQtCustomInterfaces/MultiDatasetFit.ui
               inc/MantidQtCustomInterfaces/Muon/ALCBaselineModellingView.ui
               inc/MantidQtCustomInterfaces/Muon/ALCDataLoadingView.ui
               inc/MantidQtCustomInterfaces/Muon/ALCInterface.ui
               inc/MantidQtCustomInterfaces/Muon/ALCPeakFittingView.ui
               inc/MantidQtCustomInterfaces/Muon/MuonAnalysis.ui
               inc/MantidQtCustomInterfaces/Quasi.ui
               inc/MantidQtCustomInterfaces/ReflMainWidget.ui
               inc/MantidQtCustomInterfaces/ReflOptionsDialog.ui
               inc/MantidQtCustomInterfaces/ReflWindow.ui
               inc/MantidQtCustomInterfaces/ResNorm.ui
               inc/MantidQtCustomInterfaces/SANSPlotSpecial.ui
               inc/MantidQtCustomInterfaces/SANSRunWindow.ui  
               inc/MantidQtCustomInterfaces/SANSEventSlicing.ui
               inc/MantidQtCustomInterfaces/Stretch.ui
               inc/MantidQtCustomInterfaces/MantidEV.ui
               inc/MantidQtCustomInterfaces/StepScan.ui
			   inc/MantidQtCustomInterfaces/EditLocalParameterDialog.ui
)

set ( TEST_FILES
	ALCBaselineModellingModelTest.h
	ALCBaselineModellingPresenterTest.h
	ALCDataLoadingPresenterTest.h
	ALCPeakFittingPresenterTest.h
	IO_MuonGroupingTest.h
	MuonAnalysisHelperTest.h
	ReflLegacyTransferStrategyTest.h
	ReflMainViewPresenterTest.h
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

find_Package(Threads)

target_link_libraries ( CustomInterfaces MantidQtAPI MantidWidgets QtPropertyBrowser ${CMAKE_THREAD_LIBS_INIT})

###########################################################################
# Testing
###########################################################################

add_subdirectory ( test )

###########################################################################
# Installation settings
###########################################################################

install ( TARGETS CustomInterfaces ${SYSTEM_PACKAGE_TARGET} DESTINATION ${PLUGINS_DIR}/qtplugins/mantid )
