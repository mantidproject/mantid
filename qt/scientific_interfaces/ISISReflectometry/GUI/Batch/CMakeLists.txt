set(BATCH_SRC_FILES
    BatchPresenter.cpp
    QtBatchView.cpp
    BatchJobAlgorithm.cpp
    BatchJobManager.cpp
    AlgorithmProperties.cpp
    GroupProcessingAlgorithm.cpp
    ReflAlgorithmFactory.cpp
    RowPreprocessingAlgorithm.cpp
    RowProcessingAlgorithm.cpp
    SumBanksAlgorithm.cpp
)

# Include files aren't required, but this makes them appear in Visual Studio IMPORTANT: Include files are required in
# the MOC_FILES set. Scroll down to find it.
set(BATCH_INC_FILES
    IBatchPresenter.h
    IBatchView.h
    IBatchJobAlgorithm.h
    IBatchJobManager.h
    IBatchPresenterFactory.h
    IReflAlgorithmFactory.h
    BatchPresenter.h
    BatchPresenterFactory.h
    QtBatchView.h
    BatchJobAlgorithm.h
    BatchJobManager.h
    AlgorithmProperties.h
    GroupProcessingAlgorithm.h
    ReflAlgorithmFactory.h
    RowPreprocessingAlgorithm.h
    RowProcessingAlgorithm.h
    SumBanksAlgorithm.h
)

set(BATCH_MOC_FILES QtBatchView.h)

set(BATCH_UI_FILES BatchWidget.ui)

prepend(BATCH_SRC_FILES GUI/Batch ${BATCH_SRC_FILES})
prepend(BATCH_INC_FILES GUI/Batch ${BATCH_INC_FILES})
prepend(BATCH_MOC_FILES GUI/Batch ${BATCH_MOC_FILES})
prepend(BATCH_UI_FILES GUI/Batch ${BATCH_UI_FILES})

set(BATCH_SRC_FILES
    ${BATCH_SRC_FILES}
    PARENT_SCOPE
)
set(BATCH_INC_FILES
    ${BATCH_INC_FILES}
    PARENT_SCOPE
)
set(BATCH_MOC_FILES
    ${BATCH_MOC_FILES}
    PARENT_SCOPE
)
set(BATCH_UI_FILES
    ${BATCH_UI_FILES}
    PARENT_SCOPE
)
