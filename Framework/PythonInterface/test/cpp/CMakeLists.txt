#
# C++ unit tests
#
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
  cxxtest_add_test(PythonInterfaceCppTest ${TEST_FILES})

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
