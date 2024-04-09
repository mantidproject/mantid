get_filename_component(SUB_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}" NAME)

set(SRC_FILES AbsorptionCorrections.cpp ApplyAbsorptionCorrections.cpp CalculatePaalmanPings.cpp
              ContainerSubtraction.cpp Corrections.cpp CorrectionsTab.cpp
)

set(MOC_FILES AbsorptionCorrections.h ApplyAbsorptionCorrections.h CalculatePaalmanPings.h ContainerSubtraction.h
              Corrections.h CorrectionsTab.h
)

set(UI_FILES AbsorptionCorrections.ui ApplyAbsorptionCorrections.ui CalculatePaalmanPings.ui ContainerSubtraction.ui
             Corrections.ui
)

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
