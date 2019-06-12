set(RUNS_SRC_FILES
    RunsPresenter.cpp
    RunsView.cpp
    CatalogRunNotifier.cpp
    Autoreduction.cpp)

# Include files aren't required, but this makes them appear in Visual Studio
# IMPORTANT: Include files are required in the MOC_FILES set. Scroll down to
# find it.
set(RUNS_INC_FILES
    IRunsPresenter.h
    IRunsView.h
    RunsPresenter.h
    RunsPresenterFactory.h
    RunsView.h
    IAutoreduction.h
    IRunNotifier.h
    Autoreduction.h
    CatalogRunNotifier.h)

set(RUNS_MOC_FILES RunsView.h)

set(RUNS_UI_FILES RunsWidget.ui)

prepend(RUNS_SRC_FILES GUI/Runs ${RUNS_SRC_FILES})
prepend(RUNS_INC_FILES GUI/Runs ${RUNS_INC_FILES})
prepend(RUNS_MOC_FILES GUI/Runs ${RUNS_MOC_FILES})
prepend(RUNS_UI_FILES GUI/Runs ${RUNS_UI_FILES})

set(RUNS_SRC_FILES ${RUNS_SRC_FILES} PARENT_SCOPE)
set(RUNS_INC_FILES ${RUNS_INC_FILES} PARENT_SCOPE)
set(RUNS_MOC_FILES ${RUNS_MOC_FILES} PARENT_SCOPE)
set(RUNS_UI_FILES ${RUNS_UI_FILES} PARENT_SCOPE)
