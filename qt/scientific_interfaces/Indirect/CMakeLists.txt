set(SRC_FILES
    DensityOfStates.cpp
    ILLEnergyTransfer.cpp
    ISISCalibration.cpp
    ISISDiagnostics.cpp
    IndirectAddWorkspaceDialog.cpp
    IndirectDataReduction.cpp
    IndirectDataReductionTab.cpp
    IndirectDataValidationHelper.cpp
    IndirectDiffractionReduction.cpp
    IndirectInstrumentConfig.cpp
    IndirectInterface.cpp
    IndirectMolDyn.cpp
    IndirectPlotOptionsModel.cpp
    IndirectPlotOptionsPresenter.cpp
    IndirectPlotOptionsView.cpp
    IndirectSassena.cpp
    IndirectSettings.cpp
    IndirectSettingsHelper.cpp
    IndirectSettingsModel.cpp
    IndirectSettingsPresenter.cpp
    IndirectSettingsView.cpp
    IndirectSimulation.cpp
    IndirectSimulationTab.cpp
    IndirectTab.cpp
    IndirectTransmission.cpp
    Reduction/ISISEnergyTransferValidator.cpp
    Reduction/ISISEnergyTransferModel.cpp
    Reduction/ISISEnergyTransferView.cpp
    Reduction/ISISEnergyTransferPresenter.cpp
    Reduction/ReductionAlgorithmUtils.cpp
    Tools/IndirectTools.cpp
    Tools/IndirectToolsTab.cpp
    Tools/IndirectTransmissionCalc.cpp
)

set(MOC_FILES
    DensityOfStates.h
    IAddWorkspaceDialog.h
    IIndirectSettingsView.h
    ILLEnergyTransfer.h
    IndirectAddWorkspaceDialog.h
    IndirectDataReduction.h
    IndirectDataReductionTab.h
    IndirectDiffractionReduction.h
    IndirectInstrumentConfig.h
    IndirectInterface.h
    IndirectMolDyn.h
    IndirectPlotOptionsView.h
    IndirectSassena.h
    IndirectSettings.h
    IndirectSettingsView.h
    IndirectSimulation.h
    IndirectSimulationTab.h
    IndirectTab.h
    IndirectTransmission.h
    ISISCalibration.h
    ISISDiagnostics.h
    Reduction/ISISEnergyTransferView.h
    Reduction/ISISEnergyTransferPresenter.h
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
    DensityOfStates.ui
    ILLEnergyTransfer.ui
    ISISCalibration.ui
    ISISDiagnostics.ui
    IndirectAddWorkspaceDialog.ui
    IndirectDataReduction.ui
    IndirectDiffractionReduction.ui
    IndirectInstrumentConfig.ui
    IndirectInterfaceSettings.ui
    IndirectMolDyn.ui
    IndirectPlotOptions.ui
    IndirectSassena.ui
    IndirectSimulation.ui
    IndirectTransmission.ui
    Reduction/ISISEnergyTransfer.ui
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
