if ( CXXTEST_FOUND )
    include_directories ( SYSTEM ${CXXTEST_INCLUDE_DIR} ${GMOCK_INCLUDE_DIR} ${GTEST_INCLUDE_DIR} )

    include_directories ( ../../../Framework/TestHelpers/inc
                          ../../../Framework/DataObjects/inc )

    # This variable is used within the cxxtest_add_test macro to build these helper classes into the test executable.
    # It will go out of scope at the end of this file so doesn't need un-setting
    set ( TESTHELPER_SRCS ../../../Framework/TestHelpers/src/TearDownWorld.cpp
                          ../../../Framework/TestHelpers/src/ComponentCreationHelper.cpp
                          ../../../Framework/TestHelpers/src/InstrumentCreationHelper.cpp
                          ../../../Framework/TestHelpers/src/WorkspaceCreationHelper.cpp
        )

    cxxtest_add_test ( SliceViewerTest ${TEST_FILES} )
    target_link_libraries( SliceViewerTest LINK_PRIVATE ${TCMALLOC_LIBRARIES_LINKTIME}
            SliceViewer
            Crystal
            MantidQtAPI
            API
            DataObjects
            Geometry
            HistogramData
            Kernel
            ${QT_LIBRARIES}
            ${QWT_LIBRARIES}
            ${Boost_LIBRARIES}
            ${POCO_LIBRARIES}
            ${GMOCK_LIBRARIES}
            ${GTEST_LIBRARIES} )

    add_dependencies( GUITests SliceViewerTest )

    # Add to the 'UnitTests' group in VS
    set_property( TARGET SliceViewerTest PROPERTY FOLDER "UnitTests" )
endif ()
