get_filename_component(SUB_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}" NAME)

set(SRC_FILES
    DataValidationHelper.cpp
    InelasticInterface.cpp
    InelasticTab.cpp
    InterfaceUtils.cpp
    OutputPlotOptionsModel.cpp
    OutputPlotOptionsPresenter.cpp
    OutputPlotOptionsView.cpp
    RunPresenter.cpp
    RunView.cpp
    Settings.cpp
    SettingsHelper.cpp
    SettingsModel.cpp
    SettingsPresenter.cpp
    SettingsView.cpp
    ValidationUtils.cpp
)

set(INC_FILES
    DataValidationHelper.h
    InterfaceUtils.h
    IRunSubscriber.h
    ISettingsView.h
    OutputPlotOptionsModel.h
    OutputPlotOptionsPresenter.h
    RunPresenter.h
    SettingsHelper.h
    SettingsPresenter.h
    ValidationUtils.h
)

set(MOC_FILES InelasticInterface.h InelasticTab.h OutputPlotOptionsView.h RunView.h Settings.h SettingsView.h)

set(UI_FILES InterfaceSettings.ui OutputPlotOptions.ui RunView.ui)

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
