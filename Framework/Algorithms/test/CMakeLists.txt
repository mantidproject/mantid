if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  add_compile_options(-Wno-uninitialized)
endif()

if(CXXTEST_FOUND)
  include_directories(SYSTEM ${CXXTEST_INCLUDE_DIR})
  check_include_files(stdint.h stdint)
  if(stdint)
    add_definitions(-DHAVE_STDINT_H)
  endif()

  set(TESTHELPER_SRCS PolarizationCorrections/PolarizationCorrectionsTestUtils.cpp)
  # Generate the MultiplyTest.h and DivideTest.h
  set(MULTIPLYDIVIDETEST_CLASS MultiplyTest)
  set(MULTIPLYDIVIDETEST_DO_DIVIDE false)
  configure_file(MultiplyDivideTest.in.h ${CMAKE_CURRENT_SOURCE_DIR}/MultiplyTest.h)
  set(MULTIPLYDIVIDETEST_CLASS DivideTest)
  set(MULTIPLYDIVIDETEST_DO_DIVIDE true)
  configure_file(MultiplyDivideTest.in.h ${CMAKE_CURRENT_SOURCE_DIR}/DivideTest.h)

  # Generate the PlusTest.h and MinusTest.h
  set(PLUSMINUSTEST_CLASS PlusTest)
  set(PLUSMINUSTEST_DO_PLUS true)
  configure_file(PlusMinusTest.in.h ${CMAKE_CURRENT_SOURCE_DIR}/PlusTest.h)
  set(PLUSMINUSTEST_CLASS MinusTest)
  set(PLUSMINUSTEST_DO_PLUS false)
  configure_file(PlusMinusTest.in.h ${CMAKE_CURRENT_SOURCE_DIR}/MinusTest.h)

  cxxtest_add_test(AlgorithmsTest ${TEST_FILES})
  if(WIN32)
    # On Windows Boost uuid requires the bcrypt library
    set(BCRYPT bcrypt)
  endif()
  target_link_libraries(
    AlgorithmsTest
    PRIVATE Mantid::Algorithms
            Mantid::DataHandling
            Mantid::CurveFitting
            Mantid::Nexus
            Mantid::NexusGeometry
            Mantid::Geometry
            Mantid::API
            Mantid::DataObjects
            Mantid::Kernel
            Boost::boost
            ${BCRYPT}
  )

  target_precompile_headers(
    AlgorithmsTest
    PRIVATE
    <cxxtest/WrappedTestSuite.h>
    "MantidAPI/Algorithm.h"
    "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
    <set>
    <string>
    <vector>
  )

  # Prefer imported GTest targets when available, otherwise fall back to plain targets
  if(TARGET GTest::gmock AND TARGET GTest::gtest)
    target_link_libraries(AlgorithmsTest PRIVATE GTest::gmock GTest::gtest)
  else()
    target_link_libraries(AlgorithmsTest PRIVATE gmock gtest)
  endif()

  add_framework_test_helpers(AlgorithmsTest)

  add_dependencies(AlgorithmsTest Crystal CurveFitting)
  add_dependencies(FrameworkTests AlgorithmsTest)
  # Test data
  add_dependencies(AlgorithmsTest UnitTestData)

  # Add to the 'FrameworkTests' group in VS
  set_property(TARGET AlgorithmsTest PROPERTY FOLDER "UnitTests")
endif()

# python unit tests
if(PYUNITTEST_FOUND)
  pyunittest_add_test(${CMAKE_CURRENT_SOURCE_DIR} python.algorithms ${TEST_PY_FILES})
endif()
