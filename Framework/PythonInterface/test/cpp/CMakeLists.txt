# C++ unit tests

set(TEST_FILES
    IFunction1DAdapterTest.h
    IPeakFunctionAdapterTest.h
    PropertyWithValueFactoryTest.h
    PythonAlgorithmInstantiatorTest.h
    PythonStdoutChannelTest.h
    PySequenceToVectorTest.h
    RunPythonScriptTest.h
    ToPyListTest.h
)

if(CXXTEST_FOUND)
  include_directories(SYSTEM ${CXXTEST_INCLUDE_DIR})

  set(CXXTEST_EXTRA_HEADER_INCLUDE "${CMAKE_CURRENT_LIST_DIR}/PythonInterfaceCppTestInitialization.h")

  set(_pythoninterface_test_target_name PythonInterfaceCppTest)

  cxxtest_add_test(${_pythoninterface_test_target_name} ${TEST_FILES})
  set_pythonpath_for_cxxtests(${_pythoninterface_test_target_name} "${TEST_FILES}")

  if(WIN32)
    set_target_properties(${_pythoninterface_test_target_name} PROPERTIES COMPILE_FLAGS "/DHAVE_SNPRINTF")
  endif()

  target_include_directories(${_pythoninterface_test_target_name} PUBLIC ${CMAKE_BINARY_DIR}/Framework/Types)
  target_link_libraries(
    ${_pythoninterface_test_target_name}
    LINK_PRIVATE
    API
    Geometry
    Kernel
    PythonInterfaceCore
    PythonKernelModule
    PythonAPIModule
    gmock
    gtest
    ${OpenGL_LIBRARIES}
    ${TBB_LIBRARIES}
    ${Boost_LIBRARIES}
    ${POCO_LIBRARIES}
    Python::Python
  )

  add_dependencies(FrameworkTests ${_pythoninterface_test_target_name})
  # Add to the 'UnitTests' group in VS
  set_property(TARGET ${_pythoninterface_test_target_name} PROPERTY FOLDER "UnitTests")
endif()
