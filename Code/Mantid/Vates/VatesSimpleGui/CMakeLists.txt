cmake_minimum_required( VERSION 2.8 )
project( VatesSimpleGui )
set( PROJECT_NAME VatesSimpleGui )

# Setup the include locations.
include_directories(
  inc
  ${CMAKE_CURRENT_BINARY_DIR}
  ${QWT_INCLUDE_DIR} )

# These are the C++ files to be compiled.
set( INCLUDE_FILES
  inc/AxisInformation.h
  inc/AxisInteractor.h
  inc/GeometryParser.h
  inc/Indicator.h
  inc/IView.h
  inc/ModeControlWidget.h
  inc/MpMainWindow.h
  inc/MultisliceView.h
  inc/ScalePicker.h
  inc/StandardView.h
  inc/ThreesliceView.h
  inc/TimeControlWidget.h
)
set( SOURCE_FILES
  src/AxisInformation.cpp
  src/AxisInteractor.cpp
  src/GeometryParser.cpp
  src/Indicator.cpp
  src/IView.cpp
  src/main.cpp
  src/ModeControlWidget.cpp
  src/MpMainWindow.cpp
  src/MultisliceView.cpp
  src/ScalePicker.cpp
  src/StandardView.cpp
  src/ThreesliceView.cpp
  src/TimeControlWidget.cpp
)

# These are the headers to be preprocessed using
# Qt's moc preprocessor.
qt4_wrap_cpp( MOC_SRCS
  inc/AxisInteractor.h
  inc/ModeControlWidget.h
  inc/MpMainWindow.h
  inc/MultisliceView.h
  inc/ScalePicker.h
  inc/StandardView.h
  inc/ThreesliceView.h
  inc/TimeControlWidget.h
)

# These are the ui files to be processed using
# Qt's ui file processor.
qt4_wrap_ui( UI_BUILT_SOURCES
  ui/AxisInteractor.ui
  ui/ModeControlWidget.ui
  ui/MpMainWindow.ui
  ui/MultisliceView.ui
  ui/StandardView.ui
  ui/ThreesliceView.ui
  ui/TimeControlWidget.ui
)

add_executable( ${PROJECT_NAME}
${INCLUDE_FILES} ${SOURCE_FILES} ${MOC_SRCS} ${UI_BUILT_SOURCES} )

target_link_libraries( ${PROJECT_NAME}
pqCore pqComponents pqApplicationComponents
${QT_LIBRARIES} ${QWT_LIBRARIES} ${POCO_LIBRARIES} )
