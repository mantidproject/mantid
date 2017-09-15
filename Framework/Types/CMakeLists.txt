# This is not a module, there are no source files. Types in this folder must be header-only!
set ( SRC_FILES
  src/DateAndTime.cpp
  src/TofEvent.cpp
)
set ( INC_FILES
  inc/MantidTypes/DateAndTime.h
  inc/MantidTypes/DllConfig.h
  inc/MantidTypes/SpectrumDefinition.h
  inc/MantidTypes/TofEvent.h
)

set ( TEST_FILES
  DateAndTimeTest.h
  SpectrumDefinitionTest.h
  TofEventTest.h
)

if (COVERALLS)
  foreach( loop_var ${INC_FILES})
    set_property(GLOBAL APPEND PROPERTY COVERAGE_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/${loop_var}")
  endforeach(loop_var)
endif()

add_library ( Types ${SRC_FILES} ${INC_FILES} )

# Set the name of the generated library
set_target_properties ( Types PROPERTIES OUTPUT_NAME MantidTypes
                                          COMPILE_DEFINITIONS "IN_MANTID_TYPES" )
                                          
# Add to the 'Framework' group in VS
set_property ( TARGET Types PROPERTY FOLDER "MantidFramework" )

target_link_libraries ( Types LINK_PUBLIC ${Boost_LIBRARIES} )

# Add the unit tests directory
add_subdirectory ( test )
