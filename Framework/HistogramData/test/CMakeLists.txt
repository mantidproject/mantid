if(CXXTEST_FOUND)
  include_directories(SYSTEM
                      ${CXXTEST_INCLUDE_DIR}
                      ${GMOCK_INCLUDE_DIR}
                      ${GTEST_INCLUDE_DIR}
                      ../../TestHelpers/inc)

  cxxtest_add_test(HistogramDataTest ${TEST_FILES} ${GMOCK_TEST_FILES})
  target_include_directories(HistogramDataTest SYSTEM
                             PRIVATE ${Boost_INCLUDE_DIRS})
  target_link_libraries(HistogramDataTest
                        LINK_PRIVATE
                        ${TCMALLOC_LIBRARIES_LINKTIME}
                        HistogramData
                        ${Boost_LIBRARIES}
                        ${GMOCK_LIBRARIES}
                        ${GTEST_LIBRARIES})

  add_dependencies(FrameworkTests HistogramDataTest)
  # Add to the 'FrameworkTests' group in VS
  set_property(TARGET HistogramDataTest PROPERTY FOLDER "UnitTests")
endif()
