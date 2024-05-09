get_filename_component(SUB_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}" NAME)

set(SRC_FILES DensityOfStates.cpp MolDyn.cpp Sassena.cpp Simulation.cpp SimulationTab.cpp)

set(MOC_FILES DensityOfStates.h MolDyn.h Sassena.h Simulation.h SimulationTab.h)

set(UI_FILES DensityOfStates.ui MolDyn.ui Sassena.ui Simulation.ui)

list(TRANSFORM SRC_FILES PREPEND ${SUB_DIRECTORY}/)
list(TRANSFORM MOC_FILES PREPEND ${SUB_DIRECTORY}/)
list(TRANSFORM UI_FILES PREPEND ${SUB_DIRECTORY}/)

set(ALL_SRC_FILES
    ${ALL_SRC_FILES} ${SRC_FILES}
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
