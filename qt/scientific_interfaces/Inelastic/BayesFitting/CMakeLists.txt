get_filename_component(SUB_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}" NAME)

set(SRC_FILES
    BayesFitting.cpp
    BayesFittingTab.cpp
    Quasi.cpp
    ResNormModel.cpp
    ResNormView.cpp
    ResNormPresenter.cpp
    Stretch.cpp
)

set(INC_FILES ResNormPresenter.h ResNormModel.h)

set(MOC_FILES BayesFitting.h BayesFittingTab.h Quasi.h ResNormView.h Stretch.h)

set(UI_FILES BayesFitting.ui Quasi.ui ResNorm.ui Stretch.ui)

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
