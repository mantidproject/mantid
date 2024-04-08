get_filename_component(SUB_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}" NAME)

set(SRC_FILES
    DataManipulationInterface.cpp
    ElwinTab.cpp
    ElwinTabModel.cpp
    ElwinTabView.cpp
    IqtTab.cpp
    IqtTabView.cpp
    IqtTabModel.cpp
    MomentsTab.cpp
    MomentsTabModel.cpp
    MomentsTabView.cpp
    DataManipulationTab.cpp
    SqwTab.cpp
    SqwTabModel.cpp
    SqwTabView.cpp
    SymmetriseTab.cpp
    SymmetriseTabView.cpp
    SymmetriseTabModel.cpp
)

set(INC_FILES
    ElwinTabModel.h
    ElwinTab.h
    IqtTabModel.h
    IqtTab.h
    MomentsTabModel.h
    MomentsTab.h
    SqwTabModel.h
    SqwTab.h
    SymmetriseTabModel.h
    SymmetriseTab.h
    ISqwView.h
    IMomentsView.h
    ISymmetriseView.h
    IElwinView.h
    IIqtView.h
)

set(MOC_FILES
    DataManipulationInterface.h
    ElwinTabView.h
    IqtTabView.h
    MomentsTabView.h
    SqwTabView.h
    SymmetriseTabView.h
    DataManipulationTab.h
)

set(UI_FILES DataManipulationInterface.ui ElwinTab.ui IqtTab.ui MomentsTab.ui SqwTab.ui SymmetriseTab.ui)

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
