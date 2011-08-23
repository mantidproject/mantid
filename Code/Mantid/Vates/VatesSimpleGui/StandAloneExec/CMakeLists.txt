cmake_minimum_required( VERSION 2.8 )
project( VatesSimpleGui )
set( PROJECT_NAME VatesSimpleGui )

# Setup the include locations.
include_directories(
  inc
  ${CMAKE_CURRENT_BINARY_DIR}
  ../QtWidgets/inc/MantidVatesSimpleGuiQtWidgets
  ${CMAKE_CURRENT_BINARY_DIR}/../QtWidgets
)

# These are the C++ files to be compiled.
set( INCLUDE_FILES
  inc/ColorSelectionWidget.h
  inc/ColorUpdater.h
  inc/MdViewerWidget.h
  inc/MultisliceView.h
  inc/SimpleGuiApp.h
  inc/StandardView.h
  inc/ThreesliceView.h
  inc/TimeControlWidget.h
  inc/ViewBase.h
  inc/VsgMainWindow.h
)

set( SOURCE_FILES
  src/ColorSelectionWidget.cpp
  src/ColorUpdater.cpp
  src/main.cpp
  src/MdViewerWidget.cpp
  src/MultisliceView.cpp
  src/SimpleGuiApp.cpp
  src/StandardView.cpp
  src/ThreesliceView.cpp
  src/TimeControlWidget.cpp
  src/ViewBase.cpp
  src/VsgMainWindow.cpp
)

# These are the headers to be preprocessed using
# Qt's moc preprocessor.
qt4_wrap_cpp( MOC_SRCS
  inc/ColorSelectionWidget.h
  inc/MdViewerWidget.h
  inc/MultisliceView.h
  inc/SimpleGuiApp.h
  inc/StandardView.h
  inc/ThreesliceView.h
  inc/TimeControlWidget.h
  inc/ViewBase.h
  inc/VsgMainWindow.h
)

# These are the ui files to be processed using
# Qt's ui file processor.
qt4_wrap_ui( UI_BUILT_SOURCES
  ui/ColorSelectionWidget.ui
  ui/MdViewerWidget.ui
  ui/MultisliceView.ui
  ui/StandardView.ui
  ui/ThreesliceView.ui
  ui/TimeControlWidget.ui
)

add_executable( ${PROJECT_NAME}
${INCLUDE_FILES} ${SOURCE_FILES} ${MOC_SRCS} ${UI_BUILT_SOURCES} )

target_link_libraries( ${PROJECT_NAME}
pqCore pqComponents pqApplicationComponents
${QT_LIBRARIES}
${MANTID_SUBPROJECT_LIBS}
VatesSimpleGuiQtWidgets
)

install( TARGETS VatesSimpleGui RUNTIME DESTINATION ${BIN_DIR} )
