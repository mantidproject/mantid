get_filename_component(SUB_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}" NAME)

set(SRC_FILES
    IndirectDataReduction.cpp
    IndirectDataReductionTab.cpp
    IndirectTransmission.cpp
    ILLEnergyTransfer.cpp
    ISISCalibration.cpp
    ISISDiagnostics.cpp
    ISISEnergyTransferValidator.cpp
    ISISEnergyTransferModel.cpp
    ISISEnergyTransferView.cpp
    ISISEnergyTransferPresenter.cpp
    ReductionAlgorithmUtils.cpp
)

set(INC_FILES ISISEnergyTransferData.h ISISEnergyTransferValidator.h ISISEnergyTransferModelUtils.h
              ISISEnergyTransferModel.h ReductionAlgorithmUtils.h
)

set(MOC_FILES
    IndirectDataReduction.h
    IndirectDataReductionTab.h
    IndirectTransmission.h
    ILLEnergyTransfer.h
    ISISCalibration.h
    ISISDiagnostics.h
    ISISEnergyTransferView.h
    ISISEnergyTransferPresenter.h
)

set(UI_FILES IndirectDataReduction.ui IndirectTransmission.ui ILLEnergyTransfer.ui ISISCalibration.ui
             ISISDiagnostics.ui ISISEnergyTransfer.ui
)

list(TRANSFORM SRC_FILES PREPEND ${SUB_DIRECTORY}/)
list(TRANSFORM INC_FILES PREPEND ${SUB_DIRECTORY}/)
list(TRANSFORM MOC_FILES PREPEND ${SUB_DIRECTORY}/)
list(TRANSFORM UI_FILES PREPEND ${SUB_DIRECTORY}/)

set(ALL_SRC_FILES
    ${ALL_SRC_FILES} ${SRC_FILES}
    PARENT_SCOPE
)
set(ALL_INC_FILES
    ${ALL_INC_FILES} ${INC_FILES}
    PARENT_SCOPE
)
set(ALL_MOC_FILES
    ${ALL_MOC_FILES} ${MOC_FILES}
    PARENT_SCOPE
)
set(ALL_UI_FILES
    ${ALL_UI_FILES} ${UI_FILES}
    PARENT_SCOPE
)
