set(SAVE_SRC_FILES SavePresenter.cpp QtSaveView.cpp FileSaver.cpp IFileSaver.cpp SaveAlgorithmRunner.cpp)

# Include files aren't required, but this makes them appear in Visual Studio IMPORTANT: Include files are required in
# the MOC_FILES set. Scroll down to find it.
set(SAVE_INC_FILES
    SavePresenter.h
    ISavePresenter.h
    ISaveView.h
    QtSaveView.h
    SavePresenterFactory.h
    FileSaver.h
    IFileSaver.h
    ISaveAlgorithmRunner.h
    SaveAlgorithmRunner.h
)

set(SAVE_MOC_FILES QtSaveView.h)

set(SAVE_UI_FILES SaveWidget.ui)

prepend(SAVE_SRC_FILES GUI/Save ${SAVE_SRC_FILES})
prepend(SAVE_INC_FILES GUI/Save ${SAVE_INC_FILES})
prepend(SAVE_MOC_FILES GUI/Save ${SAVE_MOC_FILES})
prepend(SAVE_UI_FILES GUI/Save ${SAVE_UI_FILES})

set(SAVE_SRC_FILES
    ${SAVE_SRC_FILES}
    PARENT_SCOPE
)
set(SAVE_INC_FILES
    ${SAVE_INC_FILES}
    PARENT_SCOPE
)
set(SAVE_MOC_FILES
    ${SAVE_MOC_FILES}
    PARENT_SCOPE
)
set(SAVE_UI_FILES
    ${SAVE_UI_FILES}
    PARENT_SCOPE
)
