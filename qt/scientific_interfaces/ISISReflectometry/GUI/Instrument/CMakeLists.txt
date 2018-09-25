set ( INSTRUMENT_SRC_FILES
  InstrumentView.cpp
  InstrumentPresenter.cpp
)

# Include files aren't required, but this makes them appear in Visual Studio
# IMPORTANT: Include files are required in the MOC_FILES set. Scroll down to find it.
set ( INSTRUMENT_INC_FILES
  InstrumentView.h
  InstrumentPresenter.h
  IInstrumentView.h
)

set ( INSTRUMENT_MOC_FILES
  InstrumentView.h
)

set ( INSTRUMENT_UI_FILES
  InstrumentWidget.ui
)

PREPEND( INSTRUMENT_SRC_FILES GUI/Instrument ${INSTRUMENT_SRC_FILES})
PREPEND( INSTRUMENT_INC_FILES GUI/Instrument ${INSTRUMENT_INC_FILES})
PREPEND( INSTRUMENT_MOC_FILES GUI/Instrument ${INSTRUMENT_MOC_FILES})
PREPEND( INSTRUMENT_UI_FILES GUI/Instrument ${INSTRUMENT_UI_FILES})

set(INSTRUMENT_SRC_FILES ${INSTRUMENT_SRC_FILES} PARENT_SCOPE)
set(INSTRUMENT_INC_FILES ${INSTRUMENT_INC_FILES} PARENT_SCOPE)
set(INSTRUMENT_MOC_FILES ${INSTRUMENT_MOC_FILES} PARENT_SCOPE)
set(INSTRUMENT_UI_FILES ${INSTRUMENT_UI_FILES} PARENT_SCOPE)
