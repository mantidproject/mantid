# C++ unit tests

set(TEST_FILES
    IFunction1DAdapterTest.h
    IPeakFunctionAdapterTest.h
    PropertyWithValueFactoryTest.h
    PythonAlgorithmInstantiatorTest.h
    PySequenceToVectorTest.h
    RunPythonScriptTest.h
    ToPyListTest.h)

if(CXXTEST_FOUND)
  include_directories(SYSTEM ${CXXTEST_INCLUDE_DIR})

  set(CXXTEST_EXTRA_HEADER_INCLUDE
      "${CMAKE_CURRENT_LIST_DIR}/GlobalInitialization.h")

  set(_test_target_name PythonInterfaceCppTest)

  cxxtest_add_test(${_test_target_name} ${TEST_FILES})

  # get mantid package root, which is the Framework/PythonInterface folder by
  # going up two levels, and then resolving to an absolute directory path
  get_filename_component(_pythoninterface_dir
                         "${CMAKE_CURRENT_SOURCE_DIR}/../../"
                         ABSOLUTE
                         DIRECTORY)

  if(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    set(
      _python_path
      ${PYTHON_XMLRUNNER_DIR};${_pythoninterface_dir};${PYUNITTEST_PYTHONPATH_EXTRA}$ENV{PYTHONPATH}
      )
    # cmake list separator and Windows environment separator are the same so
    # escape the cmake one
    string(REPLACE ";"
                   "\\;"
                   _python_path
                   "${_python_path}")
  else()
    string(REPLACE ";"
                   ":"
                   _python_path
                   "${PYUNITTEST_PYTHONPATH_EXTRA}")
    set(
      _python_path
      ${PYTHON_XMLRUNNER_DIR}:${_pythoninterface_dir}:${_python_path}:$ENV{PYTHONPATH}
      )
  endif()
  # Define the additional environment that will be added to every test
  list(APPEND _test_environment "PYTHONPATH=${_python_path}")

  # for each test that needs PYTHONPATH
  foreach(_the_test ${TEST_FILES})
    # make the name PythonInterfaceCppTest_testname
    set(_ctest_test_name "${_test_target_name}_${_the_test}")
    # Remove the trailing .h file extension
    string(REPLACE ".h"
                   ""
                   _ctest_test_name
                   ${_ctest_test_name})
    set_tests_properties(${_ctest_test_name}
                         PROPERTIES ENVIRONMENT
                                    "${_test_environment}"
                                    TIMEOUT
                                    ${TESTING_TIMEOUT})
  endforeach(_the_test ${TEST_FILES})

  if(WIN32)
    set_target_properties(PythonInterfaceCppTest
                          PROPERTIES COMPILE_FLAGS "/w44244")
  endif()
  target_link_libraries(PythonInterfaceCppTest
                        LINK_PRIVATE
                        ${TCMALLOC_LIBRARIES_LINKTIME}
                        API
                        Geometry
                        Kernel
                        PythonInterfaceCore
                        PythonKernelModule
                        PythonAPIModule
                        ${Boost_LIBRARIES}
                        ${POCO_LIBRARIES}
                        ${PYTHON_LIBRARIES})
  add_dependencies(FrameworkTests PythonInterfaceCppTest)
  # Add to the 'UnitTests' group in VS
  set_property(TARGET PythonInterfaceCppTest PROPERTY FOLDER "UnitTests")
endif()
