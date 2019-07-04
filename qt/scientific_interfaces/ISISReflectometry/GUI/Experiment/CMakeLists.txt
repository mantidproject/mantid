set(EXPERIMENT_SRC_FILES
    ExperimentView.cpp
    ExperimentPresenter.cpp
    ExperimentOptionDefaults.cpp
    InvalidDefaultsError.cpp
    PerThetaDefaultsTableValidationError.cpp
    PerThetaDefaultsTableValidator.cpp
    ExperimentOptionDefaults.h)

# Include files aren't required, but this makes them appear in Visual Studio
# IMPORTANT: Include files are required in the MOC_FILES set. Scroll down to
# find it.
set(EXPERIMENT_INC_FILES ExperimentView.h ExperimentPresenter.h
                         IExperimentView.h)

set(EXPERIMENT_MOC_FILES ExperimentView.h)

set(EXPERIMENT_UI_FILES ExperimentWidget.ui)

prepend(EXPERIMENT_SRC_FILES GUI/Experiment ${EXPERIMENT_SRC_FILES})
prepend(EXPERIMENT_INC_FILES GUI/Experiment ${EXPERIMENT_INC_FILES})
prepend(EXPERIMENT_MOC_FILES GUI/Experiment ${EXPERIMENT_MOC_FILES})
prepend(EXPERIMENT_UI_FILES GUI/Experiment ${EXPERIMENT_UI_FILES})

set(EXPERIMENT_SRC_FILES ${EXPERIMENT_SRC_FILES} PARENT_SCOPE)
set(EXPERIMENT_INC_FILES ${EXPERIMENT_INC_FILES} PARENT_SCOPE)
set(EXPERIMENT_MOC_FILES ${EXPERIMENT_MOC_FILES} PARENT_SCOPE)
set(EXPERIMENT_UI_FILES ${EXPERIMENT_UI_FILES} PARENT_SCOPE)
