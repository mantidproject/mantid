if(CXXTEST_FOUND)
  include_directories(SYSTEM
                      ${CXXTEST_INCLUDE_DIR}
                      ${GMOCK_INCLUDE_DIR}
                      ${GTEST_INCLUDE_DIR})

  include_directories(../../ScriptRepository/inc)
  include_directories(../)

  cxxtest_add_test(ScriptRepositoryTest ${TEST_FILES} ${GMOCK_TEST_FILES})
  target_link_libraries(ScriptRepositoryTest
                        LINK_PRIVATE
                        ${TCMALLOC_LIBRARIES_LINKTIME}
                        API
                        Types
                        Kernel
                        ScriptRepository
                        ${Boost_LIBRARIES}
                        ${POCO_LIBRARIES}
                        ${GMOCK_LIBRARIES}
                        ${GTEST_LIBRARIES})

  add_dependencies(FrameworkTests ScriptRepositoryTest)
  # Add to the 'FrameworkTests' group in VS
  set_property(TARGET ScriptRepositoryTest PROPERTY FOLDER "UnitTests")
endif()
