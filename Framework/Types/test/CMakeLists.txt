if(CXXTEST_FOUND)
  include_directories(SYSTEM
                      ${CXXTEST_INCLUDE_DIR}
)

  cxxtest_add_test(TypesTest ${TEST_FILES} ${GMOCK_TEST_FILES})
  target_link_libraries(TypesTest
                        LINK_PRIVATE
                        ${TCMALLOC_LIBRARIES_LINKTIME}
                        Types
                        ${Boost_LIBRARIES}
                        gmock
)

  add_dependencies(FrameworkTests TypesTest)
  # Add to the 'FrameworkTests' group in VS
  set_property(TARGET TypesTest PROPERTY FOLDER "UnitTests")
endif()
