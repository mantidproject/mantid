set ( COMMON_SRC_FILES
  CatalogRunNotifier.cpp
  Plotter.cpp
)

# Include files aren't required, but this makes them appear in Visual Studio
set ( COMMON_INC_FILES
  CatalogRunNotifier.h
  IMessageHandler.h
  IRunNotifier.h
  IPlotter.h
  Plotter.h
)

PREPEND( COMMON_SRC_FILES GUI/Common ${COMMON_SRC_FILES})
PREPEND( COMMON_INC_FILES GUI/Common ${COMMON_INC_FILES})

set(COMMON_SRC_FILES ${COMMON_SRC_FILES} PARENT_SCOPE)
set(COMMON_INC_FILES ${COMMON_INC_FILES} PARENT_SCOPE)
