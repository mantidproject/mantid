set(PREVIEW_SRC_FILES InstViewModel.cpp PreviewJobManager.cpp PreviewPresenter.cpp QtPreviewView.cpp
                      QtPreviewDockedWidgets.cpp PreviewModel.cpp
)

# Include files aren't required, but this makes them appear in Visual Studio IMPORTANT: Include files are required in
# the MOC_FILES set. Scroll down to find it.
set(PREVIEW_INC_FILES
    InstViewModel.h
    IInstViewModel.h
    IPreviewModel.h
    PreviewModel.h
    PreviewJobManager.h
    IPreviewPresenter.h
    PreviewPresenterFactory.h
    PreviewPresenter.h
    IPreviewView.h
    IPreviewDockedWidgets.h
    ROIType.h
    QtPreviewView.h
)

set(PREVIEW_MOC_FILES QtPreviewView.h QtPreviewDockedWidgets.h)

set(PREVIEW_UI_FILES PreviewWidget.ui PreviewDockedWidgets.ui)

prepend(PREVIEW_SRC_FILES GUI/Preview ${PREVIEW_SRC_FILES})
prepend(PREVIEW_INC_FILES GUI/Preview ${PREVIEW_INC_FILES})
prepend(PREVIEW_MOC_FILES GUI/Preview ${PREVIEW_MOC_FILES})
prepend(PREVIEW_UI_FILES GUI/Preview ${PREVIEW_UI_FILES})

set(PREVIEW_SRC_FILES
    ${PREVIEW_SRC_FILES}
    PARENT_SCOPE
)
set(PREVIEW_INC_FILES
    ${PREVIEW_INC_FILES}
    PARENT_SCOPE
)
set(PREVIEW_MOC_FILES
    ${PREVIEW_MOC_FILES}
    PARENT_SCOPE
)
set(PREVIEW_UI_FILES
    ${PREVIEW_UI_FILES}
    PARENT_SCOPE
)
