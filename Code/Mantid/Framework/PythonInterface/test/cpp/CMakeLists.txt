##
## C++ unit tests
##
set ( TEST_FILES
  PropertyWithValueFactoryTest.h
  PythonObjectInstantiatorTest.h
  PySequenceToVectorTest.h
  RunPythonScriptTest.h
)

if ( CXXTEST_FOUND )
  include_directories ( SYSTEM ${CXXTEST_INCLUDE_DIR} ${GMOCK_INCLUDE_DIR} ${GTEST_INCLUDE_DIR} )

  cxxtest_add_test ( PythonInterfaceCppTest ${TEST_FILES} )
  if ( WIN32 )
    set_target_properties( PythonInterfaceCppTest PROPERTIES COMPILE_FLAGS "/w44244" )
  endif ()
  target_link_libraries( PythonInterfaceCppTest PythonKernelModule PythonAPIModule ${PYTHON_LIBRARIES} )
  add_dependencies ( FrameworkTests PythonInterfaceCppTest )
  # Add to the 'UnitTests' group in VS
  set_property ( TARGET PythonInterfaceCppTest PROPERTY FOLDER "UnitTests" )
endif ()
