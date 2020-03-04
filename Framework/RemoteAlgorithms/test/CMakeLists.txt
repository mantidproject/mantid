if(CXXTEST_FOUND)
  include_directories(SYSTEM
                      ${CXXTEST_INCLUDE_DIR}
)

  cxxtest_add_test(RemoteAlgorithmsTest ${TEST_FILES})
  target_link_libraries(RemoteAlgorithmsTest
                        LINK_PRIVATE
                        ${TCMALLOC_LIBRARIES_LINKTIME}
                        API
                        Geometry
                        Kernel
                        RemoteAlgorithms
                        ${Boost_LIBRARIES}
                        ${POCO_LIBRARIES}
                        gmock
)
  add_dependencies(FrameworkTests RemoteAlgorithmsTest)
  # Test data. Not using any for now. Remember to uncomment if data is added for
  # these remote alg. tests add_dependencies ( RemoteAlgorithmsTest
  # StandardTestData )

  # Add to the 'FrameworkTests' group in VS
  set_property(TARGET RemoteAlgorithmsTest PROPERTY FOLDER "UnitTests")

endif()
