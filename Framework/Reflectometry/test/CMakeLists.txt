set(TEST_FILES
    CalculateSlitsTest.h
    ConvertToReflectometryQTest.h
    CreateTransmissionWorkspace2Test.h
    CreateTransmissionWorkspaceAuto2Test.h
    FindReflectometryLines2Test.h
    NRCalculateSlitResolutionTest.h
    ReflectometryBeamStatisticsTest.h
    ReflectometryMomentumTransferTest.h
    ReflectometryReductionOne2Test.h
    ReflectometryReductionOneAuto2Test.h
    ReflectometryReductionOneAuto3Test.h
    ReflectometryWorkflowBase2Test.h
    ReflectometryTransformKiKfTest.h
    ReflectometryTransformPTest.h
    ReflectometryTransformQxQzTest.h
    ReflectometrySumInQTest.h
    SpecularReflectionCalculateTheta2Test.h
    SpecularReflectionCalculateThetaTest.h
    SpecularReflectionPositionCorrect2Test.h
    SpecularReflectionPositionCorrectTest.h
)

set(TEST_PY_FILES ReflectometryBackgroundSubtractionTest.py ReflectometryReductionOne2Test.py)

if(CXXTEST_FOUND)
  include_directories(SYSTEM ${CXXTEST_INCLUDE_DIR})

  # This variable is used within the cxxtest_add_test macro to build these helper classes into the test executable. It
  # will go out of scope at the end of this file so doesn't need un-setting

  cxxtest_add_test(ReflectometryTest ${TEST_FILES})
  target_link_libraries(ReflectometryTest PRIVATE Mantid::Algorithms Mantid::Reflectometry gmock)

  target_include_directories(ReflectometryTest PRIVATE Mantid::Algorithms Mantid::Reflectometry)

  add_dependencies(FrameworkTests ReflectometryTest)
  # Test data
  add_dependencies(ReflectometryTest UnitTestData)
  add_framework_test_helpers(ReflectometryTest)

  # Add to the 'ReflectometryTest' group in VS
  set_property(TARGET ReflectometryTest PROPERTY FOLDER "UnitTests")
endif()

# python unit tests
pyunittest_add_test(${CMAKE_CURRENT_SOURCE_DIR} python.Reflectometry ${TEST_PY_FILES})
