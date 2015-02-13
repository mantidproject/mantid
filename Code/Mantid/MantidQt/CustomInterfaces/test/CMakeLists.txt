if ( CXXTEST_FOUND )
  include_directories ( SYSTEM ${CXXTEST_INCLUDE_DIR} ${GMOCK_INCLUDE_DIR} ${GTEST_INCLUDE_DIR} )

  include_directories( ../../../Framework/TestHelpers/inc ../../../Framework/DataObjects/inc )

  set ( TESTHELPER_SRCS ../../../Framework/TestHelpers/src/ComponentCreationHelper.cpp 
                        ../../../Framework/TestHelpers/src/WorkspaceCreationHelper.cpp
                        ../../../Framework/TestHelpers/src/ScopedFileHelper.cpp
                        ../../../Framework/TestHelpers/src/TearDownWorld.cpp
      )

  if ( GMOCK_FOUND AND GTEST_FOUND )
    cxxtest_add_test ( CustomInterfacesTest ${TEST_FILES} ${GMOCK_TEST_FILES} )
    target_link_libraries( CustomInterfacesTest CustomInterfaces DataObjects ${GMOCK_LIBRARIES} ${GTEST_LIBRARIES} )
    add_dependencies( CustomInterfacesTest MDAlgorithms )
    # Test data
    add_dependencies( CustomInterfacesTest StandardTestData )
    add_dependencies( GUITests CustomInterfacesTest )
    # Add to the 'UnitTests' group in VS
    set_property( TARGET CustomInterfacesTest PROPERTY FOLDER "UnitTests" )
  endif ()

endif ()


