cmake_minimum_required( VERSION 2.8 )
project( VatesSimpleGui )
set( PROJECT_NAME VatesSimpleGui )

# Setup the include locations.
include_directories(
  inc
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}/../QtWidgets
  ${CMAKE_CURRENT_BINARY_DIR}/../ViewWidgets
)

# These are the C++ files to be compiled.
set( INCLUDE_FILES
  inc/SimpleGuiApp.h
  inc/VsgMainWindow.h
)

set( SOURCE_FILES
  src/main.cpp
  src/SimpleGuiApp.cpp
  src/VsgMainWindow.cpp
)

# These are the headers to be preprocessed using
# Qt's moc preprocessor.
qt4_wrap_cpp( MOC_SRCS
  inc/SimpleGuiApp.h
  inc/VsgMainWindow.h
)

add_executable( ${PROJECT_NAME} ${INCLUDE_FILES} ${SOURCE_FILES} ${MOC_SRCS} )

target_link_libraries( ${PROJECT_NAME}
pqCore pqComponents pqApplicationComponents
${QT_LIBRARIES}
${MANTID_SUBPROJECT_LIBS}
VatesSimpleGuiQtWidgets
VatesSimpleGuiViewWidgets
)

install( TARGETS VatesSimpleGui RUNTIME DESTINATION ${BIN_DIR} )
