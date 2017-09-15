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

add_library ( Types INTERFACE )

# Add the unit tests directory
add_subdirectory ( test )
