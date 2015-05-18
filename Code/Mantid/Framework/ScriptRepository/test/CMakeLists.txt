if ( CXXTEST_FOUND )
  include_directories ( SYSTEM ${CXXTEST_INCLUDE_DIR} ${GMOCK_INCLUDE_DIR} ${GTEST_INCLUDE_DIR} )

  include_directories ( ../../ScriptRepository/inc )
  include_directories ( ../ )
  
  if ( GMOCK_FOUND AND GTEST_FOUND )
    cxxtest_add_test ( ScriptRepositoryTest ${TEST_FILES} ${GMOCK_TEST_FILES})   
    target_link_libraries( ScriptRepositoryTest LINK_PRIVATE ScriptRepository ${GMOCK_LIBRARIES} ${GTEST_LIBRARIES}  )
  else ()
    cxxtest_add_test ( ScriptRepositoryTest ${TEST_FILES} )
    target_link_libraries( ScriptRepositoryTest LINK_PRIVATE ScriptRepository  )
  endif()
  add_dependencies ( FrameworkTests ScriptRepositoryTest )
  # Add to the 'FrameworkTests' group in VS
  set_property ( TARGET ScriptRepositoryTest PROPERTY FOLDER "UnitTests" )
endif ()