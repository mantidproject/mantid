cmake_minimum_required( VERSION 2.8.12 )
project( VatesSimpleGui )
set( PROJECT_NAME VatesSimpleGui )

# Setup the include locations.
include_directories(
  inc
  ${CMAKE_SOURCE_DIR}/MantidQt/API/inc
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}/../QtWidgets
  ${CMAKE_CURRENT_BINARY_DIR}/../ViewWidgets
  ${CMAKE_SOURCE_DIR}/MantidQt/MantidWidgets/inc
  ${CMAKE_BINARY_DIR}/MantidQt/MantidWidgets
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

set( TEST_SUITES
    test/suite_StandAloneVsi
)

add_executable( ${PROJECT_NAME} ${INCLUDE_FILES} ${SOURCE_FILES} ${MOC_SRCS} )

target_link_libraries( ${PROJECT_NAME}
pqCore pqComponents pqApplicationComponents
${QT_LIBRARIES}
${MANTID_SUBPROJECT_LIBS}
VatesSimpleGuiQtWidgets
VatesSimpleGuiViewWidgets
)

if( SQUISH_FOUND )
  # Need to set the AUT first
  set( SQUISH_AUT VatesSimpleGui )
  set( SQUISH_AUT_PATH $<TARGET_FILE_DIR:${SQUISH_AUT}> )

  # Need to set environmental variables next
  set( auto_test_dir ${CMAKE_SOURCE_DIR} )
  string( REGEX REPLACE "Code/Mantid" "Test/AutoTestData" auto_test_dir ${auto_test_dir} )
  set( SQUISH_ENV_VARS
    PV_PLUGIN_PATH=$<TARGET_FILE_DIR:${SQUISH_AUT}>/${PVPLUGINS_DIR}/${PVPLUGINS_DIR}
    MANTID_AUTOTEST_DATA=${auto_test_dir}
    SQUISH_SCRIPT_DIR=${VSI_GLOBAL_SCRIPTS}
  )

  squish_add_test_suite( ${TEST_SUITES} )

endif()

install( TARGETS VatesSimpleGui RUNTIME DESTINATION ${BIN_DIR} )
