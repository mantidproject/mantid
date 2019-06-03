set(PLOTTING_SRC_FILES Plotter.cpp)

# Include files aren't required, but this makes them appear in Visual Studio
set(PLOTTING_INC_FILES IPlotter.h Plotter.h)

prepend(PLOTTING_SRC_FILES GUI/Plotting ${PLOTTING_SRC_FILES})
prepend(PLOTTING_INC_FILES GUI/Plotting ${PLOTTING_INC_FILES})

set(PLOTTING_SRC_FILES ${PLOTTING_SRC_FILES} PARENT_SCOPE)
set(PLOTTING_INC_FILES ${PLOTTING_INC_FILES} PARENT_SCOPE)
