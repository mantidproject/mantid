# 
# This module attempts to find the Python Sphinx
# documentation generator
#
# If the module is found the following variables are
# set:
#  SPHINX_FOUND
#  SPHINX_EXECUTABLE
#
# It also adds the macro "sphinx_add_test" for defining
# suites of documentation tests
#
#=============================================================
# SPHINX_ADD_TEST()
#   Adds a set of Sphinx doctests run using the sphinx_builder
#   It is assumed that the test files are all given as relative
#   paths ${SPHINX_SRC_DIR}
#
#   Parameters:
#       _testname_prefix :: A prefix for each test that is added to ctest, the name will be
#                           ${_testname_prefix}_TestName
#       _doctest_runner_script :: The path to the runsphinx_doctest script
#       ${ARGN} :: List of test files
#=============================================================
macro ( SPHINX_ADD_TEST _testname_prefix _doctest_runner_script )
  # The ideal would be to simply use sphinx-build everywhere but on Windows we would need
  # to call a debug version of sphinx-build, which we don't have. We therefore a
  # wrapper script, to run the test.
  # If the script finds an environment variable SPHINX_SRC_FILE then it will only process
  # that file.

  # Property for the module directory
  if ( MSVC OR CMAKE_GENERATOR STREQUAL Xcode) # both have separate configs
    set ( _multiconfig TRUE )
    set ( _module_dir ${CMAKE_BINARY_DIR}/bin/Release )
    set ( _module_dir_debug ${CMAKE_BINARY_DIR}/bin/Debug )
    set ( _working_dir ${_module_dir} )
    set ( _working_dir_debug ${_module_dir_debug} )
    if ( MSVC )
     set ( _debug_exe ${PYTHON_EXECUTABLE_DEBUG} )
     set ( _release_exe ${PYTHON_EXECUTABLE} )
    else()
     set ( _debug_exe ${PYTHON_EXECUTABLE} )
     set ( _release_exe ${PYTHON_EXECUTABLE} )
    endif()
  else()
    set ( _module_dir ${CMAKE_BINARY_DIR}/bin )
    set ( _working_dir ${_module_dir} )
    set ( _debug_exe ${PYTHON_EXECUTABLE} )
    set ( _release_exe ${PYTHON_EXECUTABLE} )
  endif()
  
  foreach ( part ${ARGN} )
      set ( _fullpath ${SPHINX_SRC_DIR}/${part} )
      get_filename_component( _filename ${part} NAME )
      get_filename_component( _docname ${part} NAME_WE )
      set ( _testname "${_testname_prefix}_${_docname}" )

      if ( _multiconfig )
        # Debug builds need to call the debug executable
        add_test ( NAME ${_testname}_Debug CONFIGURATIONS Debug
                   COMMAND ${_debug_exe} ${_doctest_runner_script} )
        set_property ( TEST ${_testname}_Debug PROPERTY
                       ENVIRONMENT "SPHINX_SRC_FILE=${_fullpath};RUNTIME_CONFIG=Debug" )
        set_property ( TEST ${_testname}_Debug PROPERTY
                       WORKING_DIRECTORY ${_working_dir} )

        # Release
        add_test ( NAME ${_testname} CONFIGURATIONS Release
                   COMMAND ${_release_exe} ${_doctest_runner_script} )
        set_property ( TEST ${_testname} PROPERTY
                       ENVIRONMENT "SPHINX_SRC_FILE=${_fullpath};RUNTIME_CONFIG=Release" )
        set_property ( TEST ${_testname} PROPERTY
                       WORKING_DIRECTORY ${_working_dir} )
      else ()
        add_test ( NAME ${_testname}
                   COMMAND ${_release_exe} ${_doctest_runner_script} )
        set_property ( TEST ${_testname} PROPERTY
                       ENVIRONMENT "SPHINX_SRC_FILE=${_fullpath}" )
        set_property ( TEST ${_testname} PROPERTY
                       WORKING_DIRECTORY ${_working_dir} )
      endif()
  endforeach ()

endmacro ()


#=============================================================
# main()
#=============================================================

find_program( SPHINX_EXECUTABLE NAME sphinx-build
  PATHS ${CMAKE_LIBRARY_PATH}/Python27/Scripts
  PATH_SUFFIXES bin
  DOC "Sphinx documentation generator"
)

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(Sphinx DEFAULT_MSG SPHINX_EXECUTABLE )

mark_as_advanced ( SPHINX_EXECUTABLE )
