# Qt 4 implementation
set(
  QT4_SRC_FILES
  AbsorptionCorrections.cpp
  ApplyAbsorptionCorrections.cpp
  CalculatePaalmanPings.cpp
  ContainerSubtraction.cpp
  CorrectionsTab.cpp
  ConvFit.cpp
  ConvFitAddWorkspaceDialog.cpp
  ConvFitDataPresenter.cpp
  ConvFitDataTablePresenter.cpp
  ConvFitModel.cpp
  DensityOfStates.cpp
  Elwin.cpp
  ILLEnergyTransfer.cpp
  ISISCalibration.cpp
  ISISDiagnostics.cpp
  ISISEnergyTransfer.cpp
  IndirectAddWorkspaceDialog.cpp
  IndirectBayes.cpp
  IndirectBayesTab.cpp
  IndirectCorrections.cpp
  IndirectDataAnalysis.cpp
  IndirectDataAnalysisTab.cpp
  IndirectDataReduction.cpp
  IndirectDataReductionTab.cpp
  IndirectDataTablePresenter.cpp
  IndirectDataValidationHelper.cpp
  IndirectDiffractionReduction.cpp
  IndirectEditResultsDialog.cpp
  IndirectFitAnalysisTab.cpp
  IndirectFitData.cpp
  IndirectFitDataPresenter.cpp
  IndirectFitDataView.cpp
  IndirectFitOutput.cpp
  IndirectFitOutputOptionsModel.cpp
  IndirectFitOutputOptionsPresenter.cpp
  IndirectFitOutputOptionsView.cpp
  IndirectFitPlotModel.cpp
  IndirectFitPlotPresenter.cpp
  IndirectFitPlotView.cpp
  IndirectFittingModel.cpp
  IndirectInstrumentConfig.cpp
  IndirectInterface.cpp
  IndirectLoadILL.cpp
  IndirectMolDyn.cpp
  IndirectMoments.cpp
  IndirectPlotOptionsModel.cpp
  IndirectPlotOptionsPresenter.cpp
  IndirectPlotOptionsView.cpp
  IndirectPlotter.cpp
  IndirectSassena.cpp
  IndirectSettings.cpp
  IndirectSettingsHelper.cpp
  IndirectSettingsModel.cpp
  IndirectSettingsPresenter.cpp
  IndirectSettingsView.cpp
  IndirectSimulation.cpp
  IndirectSimulationTab.cpp
  IndirectSpectrumSelectionPresenter.cpp
  IndirectSpectrumSelectionView.cpp
  IndirectSqw.cpp
  IndirectSymmetrise.cpp
  IndirectTab.cpp
  IndirectTools.cpp
  IndirectToolsTab.cpp
  IndirectTransmission.cpp
  IndirectTransmissionCalc.cpp
  Iqt.cpp
  IqtFit.cpp
  IqtFitModel.cpp
  JumpFit.cpp
  JumpFitAddWorkspaceDialog.cpp
  JumpFitDataPresenter.cpp
  JumpFitDataTablePresenter.cpp
  JumpFitModel.cpp
  MSDFit.cpp
  MSDFitModel.cpp
  Quasi.cpp
  ResNorm.cpp
  Stretch.cpp
)

# Include files aren't required, but this makes them appear in Visual Studio
# IMPORTANT: Include files are required in the QT4_MOC_FILES set. Scroll down to
# find it.
set(
  QT4_INC_FILES
  ConvFitModel.h
  DllConfig.h
  IndirectDataValidationHelper.h
  IndirectFitData.h
  IndirectFitOutput.h
  IndirectFitOutputOptionsModel.h
  IndirectFitPlotModel.h
  IndirectFitPlotPresenter.h
  IndirectFittingModel.h
  IndirectPlotOptionsModel.h
  IndirectSettingsHelper.h
  IndirectToolsTab.h
  IPythonRunner.h
  IqtFitModel.h
  JumpFitModel.h
  MSDFitModel.h
)

set(
  QT4_MOC_FILES
  AbsorptionCorrections.h
  ApplyAbsorptionCorrections.h
  CalculatePaalmanPings.h
  ContainerSubtraction.h
  ConvFitAddWorkspaceDialog.h
  ConvFitDataPresenter.h
  ConvFitDataTablePresenter.h
  ConvFit.h
  CorrectionsTab.h
  DensityOfStates.h
  Elwin.h
  IAddWorkspaceDialog.h
  IIndirectFitDataView.h
  IIndirectFitOutputOptionsView.h
  IIndirectFitPlotView.h
  IIndirectSettingsView.h
  ILLEnergyTransfer.h
  IndirectAddWorkspaceDialog.h
  IndirectBayes.h
  IndirectBayesTab.h
  IndirectCorrections.h
  IndirectDataAnalysis.h
  IndirectDataAnalysisTab.h
  IndirectDataReduction.h
  IndirectDataReductionTab.h
  IndirectDataTablePresenter.h
  IndirectDiffractionReduction.h
  IndirectEditResultsDialog.h
  IndirectFitAnalysisTab.h
  IndirectFitDataPresenter.h
  IndirectFitDataView.h
  IndirectFitOutputOptionsPresenter.h
  IndirectFitOutputOptionsView.h
  IndirectFitPlotPresenter.h
  IndirectFitPlotView.h
  IndirectInstrumentConfig.h
  IndirectInterface.h
  IndirectLoadILL.h
  IndirectMolDyn.h
  IndirectMoments.h
  IndirectPlotOptionsPresenter.h
  IndirectPlotOptionsView.h
  IndirectPlotter.h
  IndirectSassena.h
  IndirectSettings.h
  IndirectSettingsPresenter.h
  IndirectSettingsView.h
  IndirectSimulation.h
  IndirectSimulationTab.h
  IndirectSpectrumSelectionPresenter.h
  IndirectSpectrumSelectionView.h
  IndirectSqw.h
  IndirectSymmetrise.h
  IndirectTab.h
  IndirectTools.h
  IndirectToolsTab.h
  IndirectTransmissionCalc.h
  IndirectTransmission.h
  IqtFit.h
  Iqt.h
  ISISCalibration.h
  ISISDiagnostics.h
  ISISEnergyTransfer.h
  JumpFitAddWorkspaceDialog.h
  JumpFitDataPresenter.h
  JumpFitDataTablePresenter.h
  JumpFit.h
  LazyAsyncRunner.h
  MSDFit.h
  Quasi.h
  ResNorm.h
  Stretch.h
)

set(
  QT4_UI_FILES
  AbsorptionCorrections.ui
  ApplyAbsorptionCorrections.ui
  CalculatePaalmanPings.ui
  ContainerSubtraction.ui
  ConvFit.ui
  ConvFitAddWorkspaceDialog.ui
  DensityOfStates.ui
  Elwin.ui
  ILLEnergyTransfer.ui
  IndirectAddWorkspaceDialog.ui
  IndirectBayes.ui
  IndirectCorrections.ui
  IndirectDataAnalysis.ui
  IndirectDataReduction.ui
  IndirectDiffractionReduction.ui
  IndirectEditResultsDialog.ui
  IndirectFitDataView.ui
  IndirectFitOutputOptions.ui
  IndirectFitPreviewPlot.ui
  IndirectInterfaceSettings.ui
  IndirectInstrumentConfig.ui
  IndirectLoadILL.ui
  IndirectMolDyn.ui
  IndirectMoments.ui
  IndirectPlotOptions.ui
  IndirectSassena.ui
  IndirectSettings.ui
  IndirectSimulation.ui
  IndirectSpectrumSelector.ui
  IndirectSqw.ui
  IndirectSymmetrise.ui
  IndirectTools.ui
  IndirectTransmission.ui
  IndirectTransmissionCalc.ui
  Iqt.ui
  IqtFit.ui
  JumpFit.ui
  JumpFitAddWorkspaceDialog.ui
  MSDFit.ui
  Quasi.ui
  ResNorm.ui
  Stretch.ui
  ISISCalibration.ui
  ISISDiagnostics.ui
  ISISEnergyTransfer.ui
)

set(QT4_RES_FILES IndirectInterfaceResources.qrc)

mtd_add_qt_library(
  TARGET_NAME MantidScientificInterfacesIndirect
  QT_VERSION 4
  SRC ${QT4_SRC_FILES}
  MOC ${QT4_MOC_FILES}
  NOMOC ${QT4_INC_FILES}
  UI ${QT4_UI_FILES}
  RES ${QT4_RES_FILES}
  DEFS
    IN_MANTIDQT_INDIRECT
    PRECOMPILED
    PrecompiledHeader.h
  INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
  SYSTEM_INCLUDE_DIRS ${Boost_INCLUDE_DIRS}
  LINK_LIBS
    ${TCMALLOC_LIBRARIES_LINKTIME}
    ${CORE_MANTIDLIBS}
    ${POCO_LIBRARIES}
    ${Boost_LIBRARIES}
    ${JSONCPP_LIBRARIES}
  QT4_LINK_LIBS
    Qwt5
    Qt4::QtXml
  MTD_QT_LINK_LIBS
    MantidQtWidgetsCommon
    MantidQtWidgetsPlotting
    MantidScientificInterfacesGeneral
  INSTALL_DIR_BASE ${PLUGINS_DIR}
  OSX_INSTALL_RPATH
    @loader_path/../../Contents/MacOS
    @loader_path/../../plugins/qt4
  LINUX_INSTALL_RPATH "\$ORIGIN/../../${LIB_DIR};\$ORIGIN/../../plugins/qt4/"
)

# Qt 5 implementation
set(
  SRC_FILES
  AbsorptionCorrections.cpp
  ApplyAbsorptionCorrections.cpp
  CalculatePaalmanPings.cpp
  ContainerSubtraction.cpp
  CorrectionsTab.cpp
  DensityOfStates.cpp
  ILLEnergyTransfer.cpp
  ISISCalibration.cpp
  ISISDiagnostics.cpp
  ISISEnergyTransfer.cpp
  IndirectCorrections.cpp
  IndirectDataReduction.cpp
  IndirectDataReductionTab.cpp
  IndirectDataValidationHelper.cpp
  IndirectDiffractionReduction.cpp
  IndirectInstrumentConfig.cpp
  IndirectInterface.cpp
  IndirectLoadILL.cpp
  IndirectMolDyn.cpp
  IndirectMoments.cpp
  IndirectPlotOptionsModel.cpp
  IndirectPlotOptionsPresenter.cpp
  IndirectPlotOptionsView.cpp
  IndirectPlotter.cpp
  IndirectSassena.cpp
  IndirectSettings.cpp
  IndirectSettingsHelper.cpp
  IndirectSettingsModel.cpp
  IndirectSettingsPresenter.cpp
  IndirectSettingsView.cpp
  IndirectSimulation.cpp
  IndirectSimulationTab.cpp
  IndirectSqw.cpp
  IndirectSymmetrise.cpp
  IndirectTab.cpp
  IndirectTools.cpp
  IndirectToolsTab.cpp
  IndirectTransmission.cpp
  IndirectTransmissionCalc.cpp
)

set(
  MOC_FILES
  AbsorptionCorrections.h
  ApplyAbsorptionCorrections.h
  CalculatePaalmanPings.h
  ContainerSubtraction.h
  CorrectionsTab.h
  DensityOfStates.h
  ILLEnergyTransfer.h
  ISISCalibration.h
  ISISDiagnostics.h
  ISISEnergyTransfer.h
  IIndirectSettingsView.h
  IndirectCorrections.h
  IndirectDataReduction.h
  IndirectDataReductionTab.h
  IndirectDiffractionReduction.h
  IndirectInstrumentConfig.h
  IndirectInterface.h
  IndirectLoadILL.h
  IndirectMolDyn.h
  IndirectMoments.h
  IndirectPlotOptionsPresenter.h
  IndirectPlotOptionsView.h
  IndirectPlotter.h
  IndirectSassena.h
  IndirectSettings.h
  IndirectSettingsPresenter.h
  IndirectSettingsView.h
  IndirectSimulation.h
  IndirectSimulationTab.h
  IndirectSqw.h
  IndirectSymmetrise.h
  IndirectTab.h
  IndirectTools.h
  IndirectToolsTab.h
  IndirectTransmission.h
  IndirectTransmissionCalc.h
)

set(
  INC_FILES
  IndirectDataValidationHelper.h
  IndirectPlotOptionsModel.h
  IndirectSettingsHelper.h
  IPythonRunner.h
)

set(
  UI_FILES
  AbsorptionCorrections.ui
  ApplyAbsorptionCorrections.ui
  CalculatePaalmanPings.ui
  ContainerSubtraction.ui
  DensityOfStates.ui
  ILLEnergyTransfer.ui
  ISISCalibration.ui
  ISISDiagnostics.ui
  ISISEnergyTransfer.ui
  IndirectCorrections.ui
  IndirectDataReduction.ui
  IndirectDiffractionReduction.ui
  IndirectInstrumentConfig.ui
  IndirectInterfaceSettings.ui
  IndirectLoadILL.ui
  IndirectMolDyn.ui
  IndirectMoments.ui
  IndirectPlotOptions.ui
  IndirectSassena.ui
  IndirectSettings.ui
  IndirectSimulation.ui
  IndirectSqw.ui
  IndirectSymmetrise.ui
  IndirectTools.ui
  IndirectTransmission.ui
  IndirectTransmissionCalc.ui
)

set(RES_FILES IndirectInterfaceResources.qrc)

# XML is required to parse the settings file
find_package(Qt5 COMPONENTS Xml REQUIRED)

mtd_add_qt_library(
  TARGET_NAME MantidScientificInterfacesIndirect
  QT_VERSION 5
  SRC ${SRC_FILES}
  MOC ${MOC_FILES}
  NOMOC ${INC_FILES}
  UI ${UI_FILES}
  RES ${RES_FILES}
  DEFS
    IN_MANTIDQT_INDIRECT
    PRECOMPILED
    PrecompiledHeader.h
  INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
  LINK_LIBS
    ${TCMALLOC_LIBRARIES_LINKTIME}
    ${CORE_MANTIDLIBS}
    PythonInterfaceCore
    ${POCO_LIBRARIES}
    ${Boost_LIBRARIES}
    ${PYTHON_LIBRARIES}
    Qt5::Xml
  MTD_QT_LINK_LIBS
    MantidQtWidgetsCommon
    MantidQtWidgetsPlotting
    MantidQtWidgetsMplCpp
    MantidQtIcons
  INSTALL_DIR_BASE
    ${WORKBENCH_PLUGINS_DIR}
  OSX_INSTALL_RPATH
    @loader_path/../../Contents/MacOS
    @loader_path/../../plugins/qt5
  LINUX_INSTALL_RPATH "\$ORIGIN/../../${LIB_DIR};\$ORIGIN/../../plugins/qt5/"
)

# Testing target
add_subdirectory(test)
