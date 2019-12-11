set(OPTIONS_SRC_FILES
	QtOptionsDialogView.cpp
	OptionsDialogPresenter.cpp
	OptionsDialogModel.cpp
)

# Include files aren't required, but this makes them appear in Visual Studio
# IMPORTANT: Include files are required in the MOC_FILES set. Scroll down to
# find it.
set(OPTIONS_INC_FILES
    IOptionsDialogView.h
    QtOptionsDialogView.h
    OptionsDialogPresenter.h
    OptionsDialogModel.h
)

set(OPTIONS_MOC_FILES QtOptionsDialogView.h)

set(OPTIONS_UI_FILES QtOptionsDialogView.ui)

prepend(OPTIONS_SRC_FILES GUI/Options ${OPTIONS_SRC_FILES})
prepend(OPTIONS_INC_FILES GUI/Options ${OPTIONS_INC_FILES})
prepend(OPTIONS_MOC_FILES GUI/Options ${OPTIONS_MOC_FILES})
prepend(OPTIONS_UI_FILES GUI/Options ${OPTIONS_UI_FILES})

set(OPTIONS_SRC_FILES ${OPTIONS_SRC_FILES} PARENT_SCOPE)
set(OPTIONS_INC_FILES ${OPTIONS_INC_FILES} PARENT_SCOPE)
set(OPTIONS_MOC_FILES ${OPTIONS_MOC_FILES} PARENT_SCOPE)
set(OPTIONS_UI_FILES ${OPTIONS_UI_FILES} PARENT_SCOPE)
