get_filename_component(SUB_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}" NAME)

set(SRC_FILES
    DataManipulationInterface.cpp
    ElwinPresenter.cpp
    ElwinModel.cpp
    ElwinView.cpp
    IqtPresenter.cpp
    IqtView.cpp
    IqtModel.cpp
    MomentsPresenter.cpp
    MomentsModel.cpp
    MomentsView.cpp
    DataManipulation.cpp
    SqwPresenter.cpp
    SqwModel.cpp
    SqwView.cpp
    SymmetrisePresenter.cpp
    SymmetriseView.cpp
    SymmetriseModel.cpp
)

set(INC_FILES
    ElwinModel.h
    ElwinPresenter.h
    IqtModel.h
    IqtPresenter.h
    MomentsModel.h
    MomentsPresenter.h
    SqwModel.h
    SqwPresenter.h
    SymmetriseModel.h
    SymmetrisePresenter.h
    ISqwView.h
    IMomentsView.h
    ISymmetriseView.h
    IElwinView.h
    IIqtView.h
)

set(MOC_FILES
    DataManipulationInterface.h
    ElwinView.h
    IqtView.h
    MomentsView.h
    SqwView.h
    SymmetriseView.h
    DataManipulation.h
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
