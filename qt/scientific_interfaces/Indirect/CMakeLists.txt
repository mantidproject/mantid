set(ALL_SRC_FILES
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
)

set(ALL_MOC_FILES
    Reduction/IndirectDataReduction.h
    Reduction/IndirectDataReductionTab.h
    Reduction/IndirectTransmission.h
    Reduction/ILLEnergyTransfer.h
    Reduction/ISISCalibration.h
    Reduction/ISISDiagnostics.h
    Reduction/ISISEnergyTransferView.h
    Reduction/ISISEnergyTransferPresenter.h
)

set(ALL_INC_FILES
    DllConfig.h Reduction/ISISEnergyTransferData.h Reduction/ISISEnergyTransferValidator.h
    Reduction/ISISEnergyTransferModelUtils.h Reduction/ISISEnergyTransferModel.h Reduction/ReductionAlgorithmUtils.h
)

set(ALL_UI_FILES Reduction/IndirectDataReduction.ui Reduction/IndirectTransmission.ui Reduction/ILLEnergyTransfer.ui
                 Reduction/ISISCalibration.ui Reduction/ISISDiagnostics.ui Reduction/ISISEnergyTransfer.ui
)

set(ALL_RES_FILES ../Inelastic/Common/InterfaceResources.qrc)

add_subdirectory(Common)
add_subdirectory(Diffraction)
add_subdirectory(Simulation)
add_subdirectory(Tools)

# XML is required to parse the settings file
find_package(
  Qt5 ${QT_MIN_VERSION}
  COMPONENTS Concurrent Xml
  REQUIRED
)

mtd_add_qt_library(
  TARGET_NAME MantidScientificInterfacesIndirect
  QT_VERSION 5
  SRC ${ALL_SRC_FILES}
  MOC ${ALL_MOC_FILES}
  NOMOC ${ALL_INC_FILES}
  UI ${ALL_UI_FILES}
  RES ${ALL_RES_FILES}
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
                   MantidScientificInterfacesInelastic
  INSTALL_DIR_BASE ${WORKBENCH_PLUGINS_DIR}
  OSX_INSTALL_RPATH @loader_path/../../MacOS @loader_path/../../Frameworks @loader_path/../../plugins/qt5
  LINUX_INSTALL_RPATH "\$ORIGIN/../../${LIB_DIR};\$ORIGIN/../../plugins/qt5/"
)

# Testing target
if(MANTID_FRAMEWORK_LIB STREQUAL "BUILD")
  add_subdirectory(test)
endif()
