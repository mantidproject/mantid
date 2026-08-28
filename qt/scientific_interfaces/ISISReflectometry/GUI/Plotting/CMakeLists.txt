set(PLOTTING_SRC_FILES
    model/PlottingModel.cpp
    model/PlottingWorkspaceTree.cpp
    presenter/PlotOutputTypeProperties.cpp
    presenter/PlottingPresenter.cpp
    presenter/PlottingViewStateProvider.cpp
    presenter/QtActiveFigureMonitor.cpp
    view/QtPlottingView.cpp
    view/QtPlottingWorkspaceTreeView.cpp
    view/QtPlottingWorkspaceTreeViewAdapter.cpp
)

# Include files aren't required, but this makes them appear in Visual Studio IMPORTANT: Include files are required in
# the MOC_FILES set. Scroll down to find it.
set(PLOTTING_INC_FILES
    model/IPlottingModel.h
    model/PlottingModel.h
    model/PlottingWorkspace.h
    model/PlottingWorkspaceTree.h
    presenter/IPlottingPresenter.h
    presenter/PlotOutputTypeProperties.h
    presenter/PlottingPresenter.h
    presenter/PlottingPresenterFactory.h
    presenter/PlottingViewStateProvider.h
    presenter/QtActiveFigureMonitor.h
    view/IPlottingView.h
    view/PlottingViewState.h
    view/QtPlottingView.h
    view/QtPlottingWorkspaceTreeView.h
    view/QtPlottingWorkspaceTreeViewAdapter.h
)

set(PLOTTING_MOC_FILES presenter/QtActiveFigureMonitor.h view/QtPlottingView.h)

set(PLOTTING_UI_FILES view/PlottingWidget.ui)

prepend(PLOTTING_SRC_FILES GUI/Plotting ${PLOTTING_SRC_FILES})
prepend(PLOTTING_INC_FILES GUI/Plotting ${PLOTTING_INC_FILES})
prepend(PLOTTING_MOC_FILES GUI/Plotting ${PLOTTING_MOC_FILES})
prepend(PLOTTING_UI_FILES GUI/Plotting ${PLOTTING_UI_FILES})

set(PLOTTING_SRC_FILES
    ${PLOTTING_SRC_FILES}
    PARENT_SCOPE
)
set(PLOTTING_INC_FILES
    ${PLOTTING_INC_FILES}
    PARENT_SCOPE
)
set(PLOTTING_MOC_FILES
    ${PLOTTING_MOC_FILES}
    PARENT_SCOPE
)
set(PLOTTING_UI_FILES
    ${PLOTTING_UI_FILES}
    PARENT_SCOPE
)
