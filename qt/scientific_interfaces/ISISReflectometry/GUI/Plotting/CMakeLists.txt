set(PLOTTING_SRC_FILES PlottingPresenter.cpp QtPlottingView.cpp)

# Include files aren't required, but this makes them appear in Visual Studio IMPORTANT: Include files are required in
# the MOC_FILES set. Scroll down to find it.
set(PLOTTING_INC_FILES PlottingPresenter.h IPlottingPresenter.h IPlottingView.h QtPlottingView.h
                       PlottingPresenterFactory.h
)

set(PLOTTING_MOC_FILES QtPlottingView.h)

set(PLOTTING_UI_FILES PlottingWidget.ui)

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
