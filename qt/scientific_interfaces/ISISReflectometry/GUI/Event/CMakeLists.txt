set(EVENT_SRC_FILES EventPresenter.cpp EventView.cpp)

# Include files aren't required, but this makes them appear in Visual Studio
# IMPORTANT: Include files are required in the MOC_FILES set. Scroll down to
# find it.
set(EVENT_INC_FILES
    EventPresenter.h
    IEventPresenter.h
    IEventView.h
    EventView.h
    EventPresenterFactory.h)

set(EVENT_MOC_FILES EventView.h)

set(EVENT_UI_FILES EventWidget.ui)

prepend(EVENT_SRC_FILES GUI/Event ${EVENT_SRC_FILES})
prepend(EVENT_INC_FILES GUI/Event ${EVENT_INC_FILES})
prepend(EVENT_MOC_FILES GUI/Event ${EVENT_MOC_FILES})
prepend(EVENT_UI_FILES GUI/Event ${EVENT_UI_FILES})

set(EVENT_SRC_FILES ${EVENT_SRC_FILES} PARENT_SCOPE)
set(EVENT_INC_FILES ${EVENT_INC_FILES} PARENT_SCOPE)
set(EVENT_MOC_FILES ${EVENT_MOC_FILES} PARENT_SCOPE)
set(EVENT_UI_FILES ${EVENT_UI_FILES} PARENT_SCOPE)
