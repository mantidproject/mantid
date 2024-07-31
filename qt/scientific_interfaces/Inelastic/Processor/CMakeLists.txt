get_filename_component(SUB_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}" NAME)

set(SRC_FILES
    DataProcessor.cpp
    DataProcessorInterface.cpp
    ElwinPresenter.cpp
    ElwinModel.cpp
    ElwinView.cpp
    IqtPresenter.cpp
    IqtView.cpp
    IqtModel.cpp
    MomentsPresenter.cpp
    MomentsModel.cpp
    MomentsView.cpp
    SqwPresenter.cpp
    SqwModel.cpp
    SqwView.cpp
    SymmetrisePresenter.cpp
    SymmetriseView.cpp
    SymmetriseModel.cpp
)

set(INC_FILES
    DataProcessor.h
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

set(MOC_FILES DataProcessorInterface.h ElwinView.h IqtView.h MomentsView.h SqwView.h SymmetriseView.h)

set(UI_FILES DataProcessorInterface.ui ElwinTab.ui IqtTab.ui MomentsTab.ui SqwTab.ui SymmetriseTab.ui)

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
