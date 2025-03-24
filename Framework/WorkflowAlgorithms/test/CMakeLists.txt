set(TEST_FILES
    AlignAndFocusPowderTest.h
    ExtractQENSMembersTest.h
    IMuonAsymmetryCalculatorTest.h
    LoadEventAndCompressTest.h
    MuonProcessTest.h
    ProcessIndirectFitParametersTest.h
    SANSSolidAngleCorrectionTest.h
    SofTwoThetaTOFTest.h
    StepScanTest.h
)

set(TEST_PY_FILES EQSANSQ2DTest.py SANSBeamFluxCorrectionTest.py)

if(CXXTEST_FOUND)
  include_directories(SYSTEM ${CXXTEST_INCLUDE_DIR})

  # Make the rest of the tests This variable is used within the cxxtest_add_test macro to build these helper classes
  # into the test executable. It will go out of scope at the end of this file so doesn't need un-setting
  cxxtest_add_test(WorkflowAlgorithmsTest ${TEST_FILES})
  target_link_libraries(
    WorkflowAlgorithmsTest PRIVATE Mantid::WorkflowAlgorithms Mantid::Algorithms Mantid::DataHandling gmock
  )
  add_dependencies(WorkflowAlgorithmsTest CurveFitting)
  add_dependencies(FrameworkTests WorkflowAlgorithmsTest)
  # Test data
  add_dependencies(WorkflowAlgorithmsTest UnitTestData)
  add_framework_test_helpers(WorkflowAlgorithmsTest)

  # Add to the 'FrameworkTests' group in VS
  set_property(TARGET WorkflowAlgorithmsTest PROPERTY FOLDER "UnitTests")
endif()

# python unit tests
if(PYUNITTEST_FOUND)
  pyunittest_add_test(${CMAKE_CURRENT_SOURCE_DIR} python.WorkflowAlgorithms ${TEST_PY_FILES})
endif()
