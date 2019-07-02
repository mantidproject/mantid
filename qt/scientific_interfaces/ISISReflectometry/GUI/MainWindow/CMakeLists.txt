set(MAINWINDOW_SRC_FILES MainWindowPresenter.cpp MainWindowView.cpp)

# Include files aren't required, but this makes them appear in Visual Studio
# IMPORTANT: Include files are required in the MOC_FILES set. Scroll down to
# find it.
set(MAINWINDOW_INC_FILES
    IMainWindowPresenter.h
    IMainWindowView.h
    MainWindowPresenter.h
    MainWindowView.h)

set(MAINWINDOW_MOC_FILES MainWindowView.h)

set(MAINWINDOW_UI_FILES MainWindowWidget.ui)

prepend(MAINWINDOW_SRC_FILES GUI/MainWindow ${MAINWINDOW_SRC_FILES})
prepend(MAINWINDOW_INC_FILES GUI/MainWindow ${MAINWINDOW_INC_FILES})
prepend(MAINWINDOW_MOC_FILES GUI/MainWindow ${MAINWINDOW_MOC_FILES})
prepend(MAINWINDOW_UI_FILES GUI/MainWindow ${MAINWINDOW_UI_FILES})

set(MAINWINDOW_SRC_FILES ${MAINWINDOW_SRC_FILES} PARENT_SCOPE)
set(MAINWINDOW_INC_FILES ${MAINWINDOW_INC_FILES} PARENT_SCOPE)
set(MAINWINDOW_MOC_FILES ${MAINWINDOW_MOC_FILES} PARENT_SCOPE)
set(MAINWINDOW_UI_FILES ${MAINWINDOW_UI_FILES} PARENT_SCOPE)
