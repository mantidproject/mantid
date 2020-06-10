if(CXXTEST_FOUND)
  include_directories(SYSTEM
                      ${CXXTEST_INCLUDE_DIR}
                      ../../TestHelpers/inc)

  cxxtest_add_test(HistogramDataTest ${TEST_FILES} ${GMOCK_TEST_FILES})
  target_include_directories(HistogramDataTest SYSTEM
                             PRIVATE ${Boost_INCLUDE_DIRS})
  target_link_libraries(HistogramDataTest
                        PRIVATE
                        ${TCMALLOC_LIBRARIES_LINKTIME}
                        HistogramData
                        Kernel
                        ${Boost_LIBRARIES}
                        gmock
  )
  if(OpenMP_CXX_FOUND)
    # Access to config service is required
    target_link_libraries(HistogramDataTest PRIVATE Kernel)
  endif()

  add_dependencies(FrameworkTests HistogramDataTest)
  # Add to the 'FrameworkTests' group in VS
  set_property(TARGET HistogramDataTest PROPERTY FOLDER "UnitTests")
endif()
