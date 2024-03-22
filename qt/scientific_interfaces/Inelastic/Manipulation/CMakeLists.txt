get_filename_component(SUB_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}" NAME)

set(SRC_FILES
    InelasticDataManipulation.cpp
    InelasticDataManipulationElwinTab.cpp
    InelasticDataManipulationElwinTabModel.cpp
    InelasticDataManipulationElwinTabView.cpp
    InelasticDataManipulationIqtTab.cpp
    InelasticDataManipulationIqtTabView.cpp
    InelasticDataManipulationIqtTabModel.cpp
    InelasticDataManipulationMomentsTab.cpp
    InelasticDataManipulationMomentsTabModel.cpp
    InelasticDataManipulationMomentsTabView.cpp
    InelasticDataManipulationTab.cpp
    InelasticDataManipulationSqwTab.cpp
    InelasticDataManipulationSqwTabModel.cpp
    InelasticDataManipulationSqwTabView.cpp
    InelasticDataManipulationSymmetriseTab.cpp
    InelasticDataManipulationSymmetriseTabView.cpp
    InelasticDataManipulationSymmetriseTabModel.cpp
)

set(INC_FILES
    InelasticDataManipulationElwinTabModel.h
    InelasticDataManipulationElwinTab.h
    InelasticDataManipulationIqtTabModel.h
    InelasticDataManipulationIqtTab.h
    InelasticDataManipulationMomentsTabModel.h
    InelasticDataManipulationMomentsTab.h
    InelasticDataManipulationSqwTabModel.h
    InelasticDataManipulationSqwTab.h
    InelasticDataManipulationSymmetriseTabModel.h
    InelasticDataManipulationSymmetriseTab.h
    ISqwView.h
    IMomentsView.h
    ISymmetriseView.h
    IElwinView.h
    IIqtView.h
)

set(MOC_FILES
    InelasticDataManipulation.h
    InelasticDataManipulationElwinTabView.h
    InelasticDataManipulationIqtTabView.h
    InelasticDataManipulationMomentsTabView.h
    InelasticDataManipulationSqwTabView.h
    InelasticDataManipulationSymmetriseTabView.h
    InelasticDataManipulationTab.h
)

set(UI_FILES
    InelasticDataManipulation.ui InelasticDataManipulationElwinTab.ui InelasticDataManipulationIqtTab.ui
    InelasticDataManipulationMomentsTab.ui InelasticDataManipulationSqwTab.ui InelasticDataManipulationSymmetriseTab.ui
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
