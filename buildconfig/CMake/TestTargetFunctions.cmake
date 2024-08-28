macro(add_framework_test_helpers target_name)
  target_link_libraries(
    ${target_name} PRIVATE # This does not carry through headers transitvely
                           $<TARGET_OBJECTS:FrameworkTestHelpers> Mantid::FrameWorkTestHelpersHeaders
  )
  add_dependencies(${target_name} FrameworkTestHelpers)

  # These are from the test helpers target, unfortuantely due to https://gitlab.kitware.com/cmake/cmake/-/issues/18090
  # we have to manually specify this on a per-target basis as CMake isn't trusted to deduplicate the linked libs itself

  # This matches FrameworkTestHelper/CMakelists.txt
  target_link_libraries(
    ${target_name}
    PRIVATE Mantid::API
            Boost::boost
            Mantid::DataObjects
            Mantid::Catalog
            Mantid::DataHandling
            Mantid::Geometry
            Mantid::Kernel
            Mantid::MDAlgorithms
            Mantid::Muon
            Mantid::NexusGeometry
  )

endmacro()
