set ( BATCH_SRC_FILES
    BatchPresenter.cpp
    BatchView.cpp
)

# Include files aren't required, but this makes them appear in Visual Studio
# IMPORTANT: Include files are required in the MOC_FILES set. Scroll down to find it.
set ( BATCH_INC_FILES
    IBatchPresenter.h
    IBatchView.h
    BatchPresenter.h
    BatchPresenterFactory.h
    BatchView.h
)

set ( BATCH_MOC_FILES
    BatchView.h
)

set ( BATCH_UI_FILES
    BatchWidget.ui
)

PREPEND( BATCH_SRC_FILES GUI/Batch ${BATCH_SRC_FILES})
PREPEND( BATCH_INC_FILES GUI/Batch ${BATCH_INC_FILES})
PREPEND( BATCH_MOC_FILES GUI/Batch ${BATCH_MOC_FILES})
PREPEND( BATCH_UI_FILES GUI/Batch ${BATCH_UI_FILES})

set (BATCH_SRC_FILES ${BATCH_SRC_FILES} PARENT_SCOPE)
set (BATCH_INC_FILES ${BATCH_INC_FILES} PARENT_SCOPE)
set (BATCH_MOC_FILES ${BATCH_MOC_FILES} PARENT_SCOPE)
set (BATCH_UI_FILES ${BATCH_UI_FILES} PARENT_SCOPE)
