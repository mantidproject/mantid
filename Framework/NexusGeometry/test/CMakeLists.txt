if ( CXXTEST_FOUND )
  include_directories ( SYSTEM ${CXXTEST_INCLUDE_DIR} ${GMOCK_INCLUDE_DIR} ${GTEST_INCLUDE_DIR} )

  cxxtest_add_test ( NexusGeometryTest ${TEST_FILES})
  target_link_libraries( NexusGeometryTest LINK_PRIVATE ${TCMALLOC_LIBRARIES_LINKTIME}
            NexusGeometry
            Geometry
            Beamline
            ${NEXUS_LIBRARIES}
            ${GSL_LIBRARIES}
            ${Boost_LIBRARIES}
            ${POCO_LIBRARIES}
            ${GMOCK_LIBRARIES}
            ${GTEST_LIBRARIES} )

  add_dependencies ( FrameworkTests NexusGeometryTest )
  # Add to the 'FrameworkTests' group in VS
  set_property ( TARGET NexusGeometryTest PROPERTY FOLDER "UnitTests" )
endif ()
