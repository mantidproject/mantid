set(
  COMMON_SRC_FILES
  Plotter.cpp
  CatalogSearcher.cpp
  SearchModel.cpp
)

# Include files aren't required, but this makes them appear in Visual Studio
set(
  COMMON_INC_FILES
  IMessageHandler.h
  IPythonRunner.h
  IPlotter.h
  Plotter.h
  ISearcher.h
  CatalogSearcher.h
  SearchModel.h
)

set(COMMON_MOC_FILES SearchModel.h)

prepend(
  COMMON_SRC_FILES
  GUI/Common
  ${COMMON_SRC_FILES}
)
prepend(
  COMMON_INC_FILES
  GUI/Common
  ${COMMON_INC_FILES}
)
prepend(
  COMMON_MOC_FILES
  GUI/Common
  ${COMMON_MOC_FILES}
)

set(COMMON_SRC_FILES ${COMMON_SRC_FILES} PARENT_SCOPE)
set(COMMON_INC_FILES ${COMMON_INC_FILES} PARENT_SCOPE)
set(COMMON_MOC_FILES ${COMMON_MOC_FILES} PARENT_SCOPE)
