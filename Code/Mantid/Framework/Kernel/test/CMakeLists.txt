if ( CXXTEST_FOUND )
  include_directories ( SYSTEM ${CXXTEST_INCLUDE_DIR} ${GMOCK_INCLUDE_DIR} ${GTEST_INCLUDE_DIR} ../../TestHelpers/inc )

  cxxtest_add_test ( KernelTest ${TEST_FILES} )
  target_link_libraries( KernelTest Kernel ${GMOCK_LIBRARIES} ${GTEST_LIBRARIES} )
  add_dependencies ( FrameworkTests KernelTest )
  # Add to the 'FrameworkTests' group in VS
  set_property ( TARGET KernelTest PROPERTY FOLDER "UnitTests" )

  add_custom_command ( TARGET KernelTest POST_BUILD 
                       COMMAND ${CMAKE_COMMAND} ARGS -E copy_if_different 
                         ${CMAKE_CURRENT_SOURCE_DIR}/../../Properties/MantidTest.properties
                         ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}
                       COMMAND ${CMAKE_COMMAND} ARGS -E copy_if_different 
                         ${CMAKE_CURRENT_SOURCE_DIR}/../../Properties/MantidTest.user.properties
                         ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}
  )

endif ()

