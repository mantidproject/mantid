if(CXXTEST_FOUND)
  include_directories(SYSTEM ${CXXTEST_INCLUDE_DIR})
  check_include_files(stdint.h stdint)
  if(stdint)
    add_definitions(-DHAVE_STDINT_H)
  endif()

  # This variable is used within the cxxtest_add_test macro to build these helper classes into the test executable. It
  # will go out of scope at the end of this file so doesn't need un-setting
  cxxtest_add_test(MuonTest ${TEST_FILES})
  target_link_libraries(MuonTest PRIVATE Mantid::Algorithms Mantid::DataHandling Mantid::Muon Mantid::DataObjects)
  add_framework_test_helpers(MuonTest)

  if(TARGET GTest::gmock_main)
    target_link_libraries(MuonTest PRIVATE GTest::gmock_main)
  elseif(TARGET GTest::gmock AND TARGET GTest::gtest)
    target_link_libraries(MuonTest PRIVATE GTest::gmock GTest::gtest)
  else()
    target_link_libraries(MuonTest PRIVATE gmock gtest)
  endif()
  add_dependencies(MuonTest Crystal CurveFitting)
  add_dependencies(FrameworkTests MuonTest)
  # Test data
  add_dependencies(MuonTest UnitTestData)

  # Add to the 'MuonTest' group in VS
  set_property(TARGET MuonTest PROPERTY FOLDER "UnitTests")
endif()
