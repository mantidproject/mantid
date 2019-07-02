set(RUNS_TABLE_SRC_FILES
    RunsTablePresenter.cpp
    RunsTablePresenterFactory.cpp
    RegexRowFilter.cpp
    RunsTableView.cpp
    IRunsTableView.h
    RunsTableView.h)

# Include files aren't required, but this makes them appear in Visual Studio
# IMPORTANT: Include files are required in the MOC_FILES set. Scroll down to
# find it.
set(RUNS_TABLE_INC_FILES
    IRunsTablePresenter.h
    IRunsTableView.h
    RunsTableView.h
    RegexRowFilter.h
    RunsTablePresenter.h
    RunsTablePresenterFactory.h)

set(RUNS_TABLE_MOC_FILES RunsTableView.h)

set(RUNS_TABLE_UI_FILES RunsTableView.ui)

prepend(RUNS_TABLE_SRC_FILES GUI/RunsTable ${RUNS_TABLE_SRC_FILES})
prepend(RUNS_TABLE_INC_FILES GUI/RunsTable ${RUNS_TABLE_INC_FILES})
prepend(RUNS_TABLE_MOC_FILES GUI/RunsTable ${RUNS_TABLE_MOC_FILES})
prepend(RUNS_TABLE_UI_FILES GUI/RunsTable ${RUNS_TABLE_UI_FILES})

set(RUNS_TABLE_SRC_FILES ${RUNS_TABLE_SRC_FILES} PARENT_SCOPE)
set(RUNS_TABLE_INC_FILES ${RUNS_TABLE_INC_FILES} PARENT_SCOPE)
set(RUNS_TABLE_MOC_FILES ${RUNS_TABLE_MOC_FILES} PARENT_SCOPE)
set(RUNS_TABLE_UI_FILES ${RUNS_TABLE_UI_FILES} PARENT_SCOPE)
