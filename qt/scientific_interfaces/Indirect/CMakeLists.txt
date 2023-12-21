set(SRC_FILES
    IndirectAddWorkspaceDialog.cpp
    IndirectDataValidationHelper.cpp
    IndirectInstrumentConfig.cpp
    IndirectInterface.cpp
    IndirectPlotOptionsModel.cpp
    IndirectPlotOptionsPresenter.cpp
    IndirectPlotOptionsView.cpp
    IndirectSettings.cpp
    IndirectSettingsHelper.cpp
    IndirectSettingsModel.cpp
    IndirectSettingsPresenter.cpp
    IndirectSettingsView.cpp
    IndirectTab.cpp
    Diffraction/IndirectDiffractionReduction.cpp
    Reduction/IndirectDataReduction.cpp
    Reduction/IndirectDataReductionTab.cpp
    Reduction/IndirectTransmission.cpp
    Reduction/ILLEnergyTransfer.cpp
    Reduction/ISISCalibration.cpp
    Reduction/ISISDiagnostics.cpp
    Reduction/ISISEnergyTransferValidator.cpp
    Reduction/ISISEnergyTransferModel.cpp
    Reduction/ISISEnergyTransferView.cpp
    Reduction/ISISEnergyTransferPresenter.cpp
    Reduction/ReductionAlgorithmUtils.cpp
    Simulation/DensityOfStates.cpp
    Simulation/IndirectMolDyn.cpp
    Simulation/IndirectSassena.cpp
    Simulation/IndirectSimulation.cpp
    Simulation/IndirectSimulationTab.cpp
    Tools/IndirectTools.cpp
    Tools/IndirectToolsTab.cpp
    Tools/IndirectTransmissionCalc.cpp
)

set(MOC_FILES
    IAddWorkspaceDialog.h
    IIndirectSettingsView.h
    IndirectAddWorkspaceDialog.h
    IndirectInstrumentConfig.h
    IndirectInterface.h
    IndirectPlotOptionsView.h
    IndirectSettings.h
    IndirectSettingsView.h
    IndirectTab.h
    Diffraction/IndirectDiffractionReduction.h
    Reduction/IndirectDataReduction.h
    Reduction/IndirectDataReductionTab.h
    Reduction/IndirectTransmission.h
    Reduction/ILLEnergyTransfer.h
    Reduction/ISISCalibration.h
    Reduction/ISISDiagnostics.h
    Reduction/ISISEnergyTransferView.h
    Reduction/ISISEnergyTransferPresenter.h
    Simulation/DensityOfStates.h
    Simulation/IndirectMolDyn.h
    Simulation/IndirectSassena.h
    Simulation/IndirectSimulation.h
    Simulation/IndirectSimulationTab.h
    Tools/IndirectTools.h
    Tools/IndirectToolsTab.h
    Tools/IndirectTransmissionCalc.h
)

set(INC_FILES
    DllConfig.h
    IndirectDataValidationHelper.h
    IndirectPlotOptionsModel.h
    IndirectPlotOptionsPresenter.h
    IndirectSettingsHelper.h
    IndirectSettingsPresenter.h
    Notifier.h
    Reduction/ISISEnergyTransferData.h
    Reduction/ISISEnergyTransferValidator.h
    Reduction/ISISEnergyTransferModelUtils.h
    Reduction/ISISEnergyTransferModel.h
    Reduction/ReductionAlgorithmUtils.h
)

set(UI_FILES
    IndirectAddWorkspaceDialog.ui
    IndirectInstrumentConfig.ui
    IndirectInterfaceSettings.ui
    IndirectPlotOptions.ui
    Diffraction/IndirectDiffractionReduction.ui
    Reduction/IndirectDataReduction.ui
    Reduction/IndirectTransmission.ui
    Reduction/ILLEnergyTransfer.ui
    Reduction/ISISCalibration.ui
    Reduction/ISISDiagnostics.ui
    Reduction/ISISEnergyTransfer.ui
    Simulation/DensityOfStates.ui
    Simulation/IndirectMolDyn.ui
    Simulation/IndirectSassena.ui
    Simulation/IndirectSimulation.ui
    Tools/IndirectTools.ui
    Tools/IndirectTransmissionCalc.ui
)

set(RES_FILES IndirectInterfaceResources.qrc)

# XML is required to parse the settings file
find_package(
  Qt5 ${QT_MIN_VERSION}
  COMPONENTS Concurrent Xml
  REQUIRED
)

mtd_add_qt_library(
  TARGET_NAME MantidScientificInterfacesIndirect
  QT_VERSION 5
  SRC ${SRC_FILES}
  MOC ${MOC_FILES}
  NOMOC ${INC_FILES}
  UI ${UI_FILES}
  RES ${RES_FILES}
  DEFS IN_MANTIDQT_INDIRECT PRECOMPILED PrecompiledHeader.h
  INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
  LINK_LIBS ${CORE_MANTIDLIBS}
            Mantid::PythonInterfaceCore
            ${POCO_LIBRARIES}
            ${Boost_LIBRARIES}
            Python::Python
            Qt5::Concurrent
            Qt5::Xml
  MTD_QT_LINK_LIBS MantidQtWidgetsCommon MantidQtWidgetsPlotting MantidQtWidgetsMplCpp MantidQtIcons
  INSTALL_DIR_BASE ${WORKBENCH_PLUGINS_DIR}
  OSX_INSTALL_RPATH @loader_path/../../MacOS @loader_path/../../Frameworks @loader_path/../../plugins/qt5
  LINUX_INSTALL_RPATH "\$ORIGIN/../../${LIB_DIR};\$ORIGIN/../../plugins/qt5/"
)

# Testing target
if(MANTID_FRAMEWORK_LIB STREQUAL "BUILD")
  add_subdirectory(test)
endif()
