if ( CXXTEST_FOUND )
    include_directories ( SYSTEM ${CXXTEST_INCLUDE_DIR} ${GMOCK_INCLUDE_DIR} ${GTEST_INCLUDE_DIR} )

    include_directories ( ../../../Framework/TestHelpers/inc
                          ../../../Framework/DataObjects/inc )
    
    # This variable is used within the cxxtest_add_test macro to build these helper classes into the test executable.
    # It will go out of scope at the end of this file so doesn't need un-setting
    set ( TESTHELPER_SRCS ../../../Framework/TestHelpers/src/ComponentCreationHelper.cpp
                          ../../../Framework/TestHelpers/src/WorkspaceCreationHelper.cpp )
    
    cxxtest_add_test ( MantidQtAPITest ${TEST_FILES} )
    target_link_libraries( MantidQtAPITest MantidQtAPI DataObjects)
    add_dependencies( GUITests MantidQtAPITest )
    
    # Add to the 'UnitTests' group in VS
    set_property( TARGET MantidQtAPITest PROPERTY FOLDER "UnitTests" )
endif ()
