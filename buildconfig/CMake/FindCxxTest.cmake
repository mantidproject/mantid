# - Find CxxTest
# Find the CxxTest suite and declare a helper macro for creating unit tests
# and integrating them with CTest.
# For more details on CxxTest see http://cxxtest.tigris.org
#
# INPUT Variables
#
#   CXXTEST_USE_PYTHON
#       If true, the CXXTEST_ADD_TEST macro will use
#       the Python test generator instead of Perl.
#
# OUTPUT Variables
#
#   CXXTEST_FOUND
#       True if the CxxTest framework was found
#   CXXTEST_INCLUDE_DIR
#       Where to find the CxxTest include directory
#   CXXTEST_PERL_TESTGEN_EXECUTABLE
#       The perl-based test generator.
#   CXXTEST_PYTHON_TESTGEN_EXECUTABLE
#       The python-based test generator.
#
# MACROS for optional use by CMake users:
#
#    CXXTEST_ADD_TEST(<test_name> <gen_source_file> <input_files_to_testgen...>)
#       Creates a CxxTest runner and adds it to the CTest testing suite
#       Parameters:
#           test_name               The name of the test
#           input_files_to_testgen  The list of header files containing the
#                                   CxxTest::TestSuite's to be included in this runner
#
#    The variable TESTHELPER_SRCS can be used to pass in extra (non-test) source files
#    that should be included in the test executable.
#
#    The variable CXXTEST_EXTRA_HEADER_INCLUDE can be used to pass an additional header
#    to the --include optional of the cxxtestgen command.
#
#    The variable CXXTEST_OUTPUT_DIR can be used to specify the directory for the
#    generated files. The default is CMAKE_CURRENT_BINARY_DIR.
#
#    The variable CXXTEST_ADD_PERFORMANCE can be used to add the Performance tests to ctest.
#    This also passes --xunit-printer argument to the cxxtestgen step
#
#       #==============
#       Example Usage:
#
#           find_package(CxxTest)
#           if(CXXTEST_FOUND)
#               include_directories(${CXXTEST_INCLUDE_DIR})
#               enable_testing()
#
#               set(TESTHELPER_SRCS HelperClass1.cc HelperClass2.cc)
#               CXXTEST_ADD_TEST(unittest_foo foo_test.cc
#                                 ${CMAKE_CURRENT_SOURCE_DIR}/foo_test.h)
#               target_link_libraries(unittest_foo foo) # as needed
#           endif()
#
#              This will (if CxxTest is found):
#              1. Invoke the testgen executable to autogenerate foo_test.cc in the
#                 binary tree from "foo_test.h" in the current source directory.
#              2. Create an executable and test called unittest_foo.
#              3. Files specified in TESTHELPER_SRCS will also be compiled and linked in.
#
#      #=============
#      Example foo_test.h:
#
#          #include <cxxtest/TestSuite.h>
#
#          class MyTestSuite : public CxxTest::TestSuite
#          {
#          public:
#             void testAddition( void )
#             {
#                TS_ASSERT( 1 + 1 > 1 );
#                TS_ASSERT_EQUALS( 1 + 1, 2 );
#             }
#          };
#

#=============================================================================
# Copyright 2008-2009 Kitware, Inc.
# Copyright 2008-2009 Philip Lowman <philip@yhbt.com>
#
# Distributed under the OSI-approved BSD License (the "License");
# see accompanying file Copyright.txt for details.
#
# This software is distributed WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the License for more information.
#=============================================================================
# (To distributed this file outside of CMake, substitute the full
#  License text for the above reference.)

# Version 1.2 (3/2/08)
#     Included patch from Tyler Roscoe to have the perl & python binaries
#     detected based on CXXTEST_INCLUDE_DIR
# Version 1.1 (2/9/08)
#     Clarified example to illustrate need to call target_link_libraries()
#     Changed commands to lowercase
#     Added licensing info
# Version 1.0 (1/8/08)
#     Fixed CXXTEST_INCLUDE_DIRS so it will work properly
#     Eliminated superfluous CXXTEST_FOUND assignment
#     Cleaned up and added more documentation

include ( PrecompiledHeaderCommands )

#=============================================================
# CXXTEST_ADD_TEST (public macro to add unit tests)
#=============================================================
macro(CXXTEST_ADD_TEST _cxxtest_testname)
  # output directory
    set (_cxxtest_output_dir ${CMAKE_CURRENT_BINARY_DIR})
    if (CXXTEST_OUTPUT_DIR)
      set (_cxxtest_output_dir ${CXXTEST_OUTPUT_DIR})
      if ( NOT IS_DIRECTORY "${_cxxtest_output_dir}")
        file ( MAKE_DIRECTORY "${_cxxtest_output_dir}" )
      endif()
    endif()
    # determine the cpp filename
    set(_cxxtest_real_outfname ${_cxxtest_output_dir}/${_cxxtest_testname}_runner.cpp)
    # add additional include if requested
    if(CXXTEST_EXTRA_HEADER_INCLUDE)
      set(_cxxtest_include  --include ${CXXTEST_EXTRA_HEADER_INCLUDE})
    endif()
    # Use CxxTest XUnit formatter to record timings for performance tests
    if(CXXTEST_ADD_PERFORMANCE)
      set(_printer "--xunit-printer")
    else()
      set(_printer "--error-printer")
    endif()

    add_custom_command(
        OUTPUT  ${_cxxtest_real_outfname}
        DEPENDS ${CXXTEST_PYTHON_FILES} ${CXXTEST_TESTGEN_EXECUTABLE}
        COMMAND ${Python_EXECUTABLE} ${CXXTEST_TESTGEN_EXECUTABLE} --root
         ${_printer} --world ${_cxxtest_testname} ${_cxxtest_include} -o ${_cxxtest_real_outfname}
    )
    set_source_files_properties(${_cxxtest_real_outfname} PROPERTIES GENERATED true)

    # convert the header files to have full path
    set (_cxxtest_cpp_files ${_cxxtest_real_outfname} )
    set (_cxxtest_h_files )
    foreach (part ${ARGN})
      get_filename_component(_cxxtest_cpp ${part} NAME)
      string ( REPLACE ".h" ".cpp" _cxxtest_cpp ${_cxxtest_cpp} )
      set ( _cxxtest_cpp "${_cxxtest_output_dir}/${_cxxtest_cpp}" )
      set ( _cxxtest_h "${CMAKE_CURRENT_SOURCE_DIR}/${part}" )

      add_custom_command(
        OUTPUT  ${_cxxtest_cpp}
        DEPENDS ${_cxxtest_h} ${CXXTEST_PYTHON_FILES} ${CXXTEST_TESTGEN_EXECUTABLE}
        COMMAND ${Python_EXECUTABLE} ${CXXTEST_TESTGEN_EXECUTABLE} --part
        --world ${_cxxtest_testname} -o ${_cxxtest_cpp} ${_cxxtest_h}
	)

      set_source_files_properties(${_cxxtest_cpp} PROPERTIES GENERATED true)

      set (_cxxtest_cpp_files ${_cxxtest_cpp} ${_cxxtest_cpp_files})
      set (_cxxtest_h_files ${part} ${_cxxtest_h_files})
    endforeach (part ${ARGN})

    if( EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/PrecompiledHeader.h )
      enable_precompiled_headers( PrecompiledHeader.h  _cxxtest_cpp_files )
    ENDIF ()

    # define the test executable and exclude it from the all target
    # The TESTHELPER_SRCS variable can be set outside the macro and used to pass in test helper classes
    add_executable(${_cxxtest_testname} EXCLUDE_FROM_ALL ${_cxxtest_cpp_files} ${_cxxtest_h_files} ${TESTHELPER_SRCS} )

    set (_misc_bin ${THIRD_PARTY_DIR}/bin)
    set (_qt5_bin ${THIRD_PARTY_DIR}/lib/qt5/bin ${THIRD_PARTY_DIR}/lib/qt5/lib)
    set (_qt_qpa_platform_plugin ${THIRD_PARTY_DIR}/lib/qt5/plugins)
    set (_python_home ${THIRD_PARTY_DIR}/lib/python3.8)
    # Note: %PATH% isn't understood by cmake but it is by Visual Studio\Windows where it gets used
    set_target_properties(${_cxxtest_testname} PROPERTIES VS_DEBUGGER_ENVIRONMENT "PATH=${_misc_bin};${_qt5_bin};${_python_home};${_python_home}/Scripts;%PATH%\n\
QT_QPA_PLATFORM_PLUGIN_PATH=${_qt_qpa_platform_plugin}\n\
PYTHONHOME=${_python_home}")

    # only the package wide test is added to check
    add_dependencies(check ${_cxxtest_testname})

    # add each separate test to ctest
    foreach ( part ${ARGN} )
      # The filename without extension = The suite name.
      get_filename_component(_suitename ${part} NAME_WE )
      set( _cxxtest_separate_name "${_cxxtest_testname}_${_suitename}")
      add_test ( NAME ${_cxxtest_separate_name}
                COMMAND ${CMAKE_COMMAND} -E chdir "${CMAKE_BINARY_DIR}/bin/Testing"
            $<TARGET_FILE:${_cxxtest_testname}> ${_suitename} )

      set_tests_properties ( ${_cxxtest_separate_name} PROPERTIES
                             TIMEOUT ${TESTING_TIMEOUT} )
      if(WIN32)
        set_property( TEST ${_cxxtest_separate_name} APPEND PROPERTY
           ENVIRONMENT "QT_QPA_PLATFORM_PLUGIN_PATH=${_qt_qpa_platform_plugin}")
        set_property( TEST ${_cxxtest_separate_name} APPEND PROPERTY
           ENVIRONMENT "PYTHONHOME=${_python_home}")
        set (_new_path ${_misc_bin} ${_qt5_bin} ${_python_home} ${_python_home}/Scripts $ENV{PATH})
        # the value used for PATH has to have explicit semi colons for some reason
        string(REPLACE ";" "\;" _new_path "${_new_path}")
        set_property( TEST ${_cxxtest_separate_name} APPEND PROPERTY
            ENVIRONMENT "PATH=${_new_path}")
      endif()
      if (CXXTEST_ADD_PERFORMANCE)
        # ------ Performance test version -------
        # Name of the possibly-existing Performance test suite
        set( _performance_suite_name "${_suitename}Performance" )
        # Read the contents of the header file
          FILE( READ ${part} _file_contents )
        # Is that suite defined in there at all?
        STRING(REGEX MATCH ${_performance_suite_name} _search_res ${_file_contents} )
        if (NOT "${_search_res}" STREQUAL "")
          set( _cxxtest_separate_name "${_cxxtest_testname}_${_performance_suite_name}")
          add_test ( NAME ${_cxxtest_separate_name}
                    COMMAND ${CMAKE_COMMAND} -E chdir "${CMAKE_BINARY_DIR}/bin/Testing"
                $<TARGET_FILE:${_cxxtest_testname}> ${_performance_suite_name} )
          set_tests_properties ( ${_cxxtest_separate_name} PROPERTIES
                                TIMEOUT ${TESTING_TIMEOUT} )
        endif ()
      endif ()

      set( SUPPRESSIONS_DIR "${CMAKE_SOURCE_DIR}/tools/Sanitizer" )
      if ( USE_SANITIZERS_LOWER STREQUAL "address" )
        # See dev docs on sanitizers for details on why verify_asan_link is false
        # Trying to quote these options causes the quotation to be forwarded
        set(_ASAN_OPTS "suppressions=${SUPPRESSIONS_DIR}/Address.supp:detect_stack_use_after_return=true")
        set_property( TEST ${_cxxtest_separate_name} APPEND PROPERTY
          ENVIRONMENT ASAN_OPTIONS=${_ASAN_OPTS} )

        set( _LSAN_OPTS "suppressions=${SUPPRESSIONS_DIR}/Leak.supp" )
          set_property( TEST ${_cxxtest_separate_name} APPEND PROPERTY
          ENVIRONMENT LSAN_OPTIONS=${_LSAN_OPTS} )

      endif()


    endforeach ( part ${ARGN} )
endmacro(CXXTEST_ADD_TEST)

#=============================================================
# main()
#=============================================================

find_path(CXXTEST_INCLUDE_DIR cxxtest/TestSuite.h
          PATHS ${PROJECT_SOURCE_DIR}/Testing/Tools/cxxtest
	        ${PROJECT_SOURCE_DIR}/../Testing/Tools/cxxtest
                NO_DEFAULT_PATH )

find_program(CXXTEST_TESTGEN_EXECUTABLE python/scripts/cxxtestgen
             PATHS ${CXXTEST_INCLUDE_DIR})

file(GLOB_RECURSE CXXTEST_PYTHON_FILES
     ${PROJECT_SOURCE_DIR}/Testing/Tools/cxxtest/python/*.py)

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(CxxTest DEFAULT_MSG CXXTEST_INCLUDE_DIR)

set(CXXTEST_INCLUDE_DIRS ${CXXTEST_INCLUDE_DIR})

mark_as_advanced ( CXXTEST_INCLUDE_DIR CXXTEST_TESTGEN_EXECUTABLE CXXTEST_PYTHON_FILES )
