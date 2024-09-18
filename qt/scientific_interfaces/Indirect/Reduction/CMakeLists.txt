get_filename_component(SUB_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}" NAME)

set(SRC_FILES
    DataReduction.cpp
    DataReductionTab.cpp
    ILLEnergyTransfer.cpp
    ISISCalibration.cpp
    ISISDiagnostics.cpp
    ISISEnergyTransferModel.cpp
    ISISEnergyTransferPresenter.cpp
    ISISEnergyTransferValidator.cpp
    ISISEnergyTransferView.cpp
    ReductionAlgorithmUtils.cpp
    Transmission.cpp
)

set(INC_FILES ISISEnergyTransferData.h ISISEnergyTransferValidator.h ISISEnergyTransferModelUtils.h
              ISISEnergyTransferModel.h ISISEnergyTransferPresenter.h ReductionAlgorithmUtils.h
)

set(MOC_FILES
    DataReduction.h
    DataReductionTab.h
    ILLEnergyTransfer.h
    ISISCalibration.h
    ISISDiagnostics.h
    ISISEnergyTransferView.h
    Transmission.h
)

set(UI_FILES DataReduction.ui ILLEnergyTransfer.ui ISISCalibration.ui ISISDiagnostics.ui ISISEnergyTransfer.ui
             Transmission.ui
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
