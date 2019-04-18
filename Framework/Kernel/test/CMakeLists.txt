if(CXXTEST_FOUND)
  include_directories(SYSTEM
                      ${CXXTEST_INCLUDE_DIR}
                      ${GMOCK_INCLUDE_DIR}
                      ${GTEST_INCLUDE_DIR}
                      ../../TestHelpers/inc)

  # This is required to pick up NexusTestHelper
  include_directories(../../TestHelpers/inc)
  # This variable is used within the cxxtest_add_test macro to build this helper
  # class into the test executable. It will go out of scope at the end of this
  # file so doesn't need un-setting
  set(TESTHELPER_SRCS ../../TestHelpers/src/NexusTestHelper.cpp
      ../../TestHelpers/src/LoggingCleaner.cpp)

  cxxtest_add_test(KernelTest ${TEST_FILES})

  target_link_libraries(KernelTest
                        LINK_PRIVATE
                        ${TCMALLOC_LIBRARIES_LINKTIME}
                        Types
                        Kernel
                        ${NEXUS_LIBRARIES}
                        ${Boost_LIBRARIES}
                        ${POCO_LIBRARIES}
                        ${GMOCK_LIBRARIES}
                        ${GTEST_LIBRARIES}
                        ${JSONCPP_LIBRARIES}
                        ${TBB_LIBRARIES}
                        ${TBB_MALLOC_LIBRARIES})
  add_dependencies(FrameworkTests KernelTest)
  # Test data
  add_dependencies(KernelTest StandardTestData)

  # Add to the 'FrameworkTests' group in VS
  set_property(TARGET KernelTest PROPERTY FOLDER "UnitTests")

  add_custom_command(
    TARGET KernelTest POST_BUILD
    COMMAND
      ${CMAKE_COMMAND}
      ARGS -E
           copy_if_different
           ${CMAKE_CURRENT_SOURCE_DIR}/../../Properties/MantidTest.properties
           ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}
    COMMAND
      ${CMAKE_COMMAND}
      ARGS
        -E
        copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/../../Properties/MantidTest.user.properties
        ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR})

endif()
