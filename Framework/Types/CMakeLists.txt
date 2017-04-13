# This is not a module, there are no source files. Types in this folder must be header-only!

set ( INC_FILES
  inc/MantidTypes/SpectrumDefinition.h
)

set ( TEST_FILES
  SpectrumDefinitionTest.h
)

if (COVERALLS)
  foreach( loop_var ${INC_FILES})
    set_property(GLOBAL APPEND PROPERTY COVERAGE_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/${loop_var}")
  endforeach(loop_var)
endif()

# Add the target for this directory
add_library ( Types ${INC_FILES} )
# Set the name of the generated library
set_target_properties ( Types PROPERTIES OUTPUT_NAME MantidTypes
        COMPILE_DEFINITIONS IN_MANTID_TYPES LINKER_LANGUAGE CXX )

# Add the unit tests directory
add_subdirectory ( test )
