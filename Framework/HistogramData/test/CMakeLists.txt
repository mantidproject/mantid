if ( CXXTEST_FOUND )
  include_directories ( SYSTEM ${CXXTEST_INCLUDE_DIR} ${GMOCK_INCLUDE_DIR} ${GTEST_INCLUDE_DIR} ../../TestHelpers/inc)

  cxxtest_add_test ( HistogramDataTest ${TEST_FILES} ${GMOCK_TEST_FILES})
  target_include_directories ( HistogramDataTest SYSTEM PRIVATE ${Boost_INCLUDE_DIRS} )
  target_link_libraries( HistogramDataTest LINK_PRIVATE ${TCMALLOC_LIBRARIES_LINKTIME}
    HistogramData
    ${Boost_LIBRARIES}
    ${GMOCK_LIBRARIES}
    ${GTEST_LIBRARIES} )

  add_executable ( GoogleHistogramDataTest google_test.cpp ${GMOCK_TEST_FILES})
  target_include_directories ( GoogleHistogramDataTest SYSTEM PRIVATE ${Boost_INCLUDE_DIRS} )
  target_link_libraries( GoogleHistogramDataTest
    LINK_PRIVATE
    gtest_main
    ${TCMALLOC_LIBRARIES_LINKTIME}
    HistogramData
    ${Boost_LIBRARIES}
    ${GMOCK_LIBRARIES}
    ${GTEST_LIBRARIES} )

  # gtest_add_tests ( TARGET GoogleHistogramDataTest ) # CMake 3.9
  find_package (GTest REQUIRED)
  # auto discover all tests, runs them individually via --gtest_filter, so this could be slow for general use
  GTEST_ADD_TESTS ( GoogleHistogramDataTest "--gtest_output=xml:TEST-GoogleHistogramDataTest.xml" AUTO )

  add_test(NAME GTestTest1 COMMAND GoogleHistogramDataTest --gtest_filter=GTestTest1.* --gtest_output=xml:TEST-GTestTest1.xml)
  add_test(NAME GTestTest2 COMMAND GoogleHistogramDataTest --gtest_filter=GTestTest2.* --gtest_output=xml:TEST-GTestTest2.xml)

  add_dependencies ( FrameworkTests HistogramDataTest )
  # Add to the 'FrameworkTests' group in VS
  set_property ( TARGET HistogramDataTest PROPERTY FOLDER "UnitTests" )
endif ()
