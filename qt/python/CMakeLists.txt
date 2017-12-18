# This file manages building/installation of the mantidqt and mantidqtpython
# Python wrappers.
#

# Legacy wrappers for MantidPlot
add_subdirectory ( mantidqtpython )

###########################################################################
# mantidqt
###########################################################################

if ( ENABLE_WORKBENCH )

  # Configure utils.qt.plugins file for build. It is placed in the source
  # directory and added to the .gitignore for simplicity.
  if ( WIN32 )
    set ( QT_PLUGINS_PATH "${THIRD_PARTY_DIR}/lib/qt%V/plugins" )
  endif ()
    configure_file ( mantidqt/utils/qt/plugins.py.in
      ${CMAKE_CURRENT_SOURCE_DIR}/mantidqt/utils/qt/plugins.py
  )

  # Create egg link to binary output directory for mantidqt
  add_python_package ( mantidqt )

  # Now add any compiled sip targets
  add_subdirectory ( mantidqt/widgets )

  # Setup dependency chain
  add_dependencies ( mantidqt mantidqt_widgetscoreqt5 )

  ##########################################################################
  # Testing
  ##########################################################################

  # ctest targets
  set ( PYTHON_TEST_FILES
    mantidqt/test/algorithmselector_test.py
    mantidqt/test/test_import.py
    mantidqt/utils/test/test_qt_utils.py
    mantidqt/widgets/test/test_messagedisplay.py
 )

  # Tests
  pyunittest_add_test ( ${CMAKE_CURRENT_SOURCE_DIR}
    mantidqt ${PYTHON_TEST_FILES}
  )

  # No package installation yet...
  # Configure utils.qt.plugins file for install
endif ()
