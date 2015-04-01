if ( CXXTEST_FOUND )
  include_directories ( SYSTEM ${CXXTEST_INCLUDE_DIR} ${GMOCK_INCLUDE_DIR} ${GTEST_INCLUDE_DIR} )
  check_include_files ( stdint.h stdint )
  if ( stdint )
     add_definitions ( -DHAVE_STDINT_H )
   endif ()
  check_include_files ( cstdint.hpp boost_stdint)
  if ( boost_stdint )
    add_definitions ( -DBOOST_CSTDINT_HPP )
  endif ()


  # Generate the MultiplyTest.h and DivideTest.h
  set ( MULTIPLYDIVIDETEST_CLASS MultiplyTest )
  set ( MULTIPLYDIVIDETEST_DO_DIVIDE false )
  configure_file ( MultiplyDivideTest.in.h      ${CMAKE_CURRENT_SOURCE_DIR}/MultiplyTest.h)
  set ( MULTIPLYDIVIDETEST_CLASS DivideTest )
  set ( MULTIPLYDIVIDETEST_DO_DIVIDE true )
  configure_file ( MultiplyDivideTest.in.h      ${CMAKE_CURRENT_SOURCE_DIR}/DivideTest.h)

  # Generate the PlusTest.h and MinusTest.h
  set ( PLUSMINUSTEST_CLASS PlusTest )
  set ( PLUSMINUSTEST_DO_PLUS true )
  configure_file ( PlusMinusTest.in.h      ${CMAKE_CURRENT_SOURCE_DIR}/PlusTest.h)
  set ( PLUSMINUSTEST_CLASS MinusTest )
  set ( PLUSMINUSTEST_DO_PLUS false )
  configure_file ( PlusMinusTest.in.h      ${CMAKE_CURRENT_SOURCE_DIR}/MinusTest.h)
  
  # Make the rest of the tests
  include_directories ( ../../CurveFitting/inc ../../DataHandling/inc ../../Nexus/inc ../../TestHelpers/inc )
  # This variable is used within the cxxtest_add_test macro to build these helper classes into the test executable.
  # It will go out of scope at the end of this file so doesn't need un-setting
  set ( TESTHELPER_SRCS ../../TestHelpers/src/TearDownWorld.cpp
                        ../../TestHelpers/src/ComponentCreationHelper.cpp
                        ../../TestHelpers/src/WorkspaceCreationHelper.cpp
                        ../../TestHelpers/src/SANSInstrumentCreationHelper.cpp
                        ../../TestHelpers/src/MDEventsTestHelper.cpp
                        ../../TestHelpers/src/ScopedFileHelper.cpp
                        )
  cxxtest_add_test ( AlgorithmsTest ${TEST_FILES} )
  target_link_libraries ( AlgorithmsTest Algorithms DataHandling ${GMOCK_LIBRARIES} )
  add_dependencies ( AlgorithmsTest Crystal CurveFitting )
  add_dependencies ( FrameworkTests AlgorithmsTest )
  # Test data
  add_dependencies ( AlgorithmsTest StandardTestData )

  # Add to the 'FrameworkTests' group in VS
  set_property ( TARGET AlgorithmsTest PROPERTY FOLDER "UnitTests" )
endif ()

# python unit tests
if (PYUNITTEST_FOUND)
  pyunittest_add_test( ${CMAKE_CURRENT_SOURCE_DIR} AlgorithmsPythonTest ${TEST_PY_FILES} )
endif ()

