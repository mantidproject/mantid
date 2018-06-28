set ( RUNS_TABLE_SRC_FILES
    RunsTablePresenter.cpp
    RunsTablePresenterFactory.cpp
    RegexRowFilter.cpp
    RowLocation.cpp
    RunsTableView.cpp
    IRunsTableView.h
    RunsTableView.h
    PARENT_SCOPE
)

# Include files aren't required, but this makes them appear in Visual Studio
# IMPORTANT: Include files are required in the MOC_FILES set. Scroll down to find it.
set ( RUNS_TABLE_INC_FILES
    IRunsTableView.h
    RunsTableView.h
    RegexRowFilter.h
    RowLocation.h
    RunsTablePresenter.h
    RunsTablePresenterFactory.h
    PARENT_SCOPE
)

set ( RUNS_TABLE_MOC_FILES
    RunsTableView.h
    PARENT_SCOPE
)

set ( RUNS_TABLE_UI_FILES
    RunsTableView.ui
    PARENT_SCOPE
)
