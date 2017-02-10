if ( CXXTEST_FOUND )
  include_directories ( SYSTEM ${CXXTEST_INCLUDE_DIR} ${GMOCK_INCLUDE_DIR} ${GTEST_INCLUDE_DIR} )

  include_directories( ../../../Framework/TestHelpers/inc ../../../Framework/DataObjects/inc )

  set ( TESTHELPER_SRCS ../../../Framework/TestHelpers/src/ComponentCreationHelper.cpp
                        ../../../Framework/TestHelpers/src/InstrumentCreationHelper.cpp
                        ../../../Framework/TestHelpers/src/WorkspaceCreationHelper.cpp
      )

  cxxtest_add_test ( MantidWidgetsTest ${TEST_FILES} )
  target_link_libraries( MantidWidgetsTest LINK_PRIVATE ${TCMALLOC_LIBRARIES_LINKTIME}
            MantidWidgets
            API
            DataObjects
            Geometry
            HistogramData
            Kernel
            ${QT_LIBRARIES}
            ${Boost_LIBRARIES}
            ${POCO_LIBRARIES}
            ${GMOCK_LIBRARIES}
            ${GTEST_LIBRARIES}
            )

  # Add to the 'UnitTests' group in VS
  set_property( TARGET MantidWidgetsTest PROPERTY FOLDER "UnitTests" )
endif ()

if ( PYUNITTEST_FOUND )
  pyunittest_add_test (${CMAKE_CURRENT_SOURCE_DIR} MantidWidgetsTest ${TEST_PY_FILES} )
endif ()
