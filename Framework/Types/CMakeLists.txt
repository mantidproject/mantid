# This is not a module, there are no source files. Types in this folder must be header-only!

set ( SRC_FILES 
  src/Core/DateAndTime.cpp
  src/Event/TofEvent.cpp
)

set ( INC_FILES
  inc/MantidTypes/Core/DateAndTime.h
  inc/MantidTypes/SpectrumDefinition.h
  inc/MantidTypes/Event/TofEvent.h
)

set ( TEST_FILES
  SpectrumDefinitionTest.h
  DateAndTimeTest.h
  TofEventTest.h
)

if (COVERALLS)
  foreach( loop_var ${INC_FILES})
    set_property(GLOBAL APPEND PROPERTY COVERAGE_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/${loop_var}")
  endforeach(loop_var)
endif()

add_library ( Types ${SRC_FILES} ${INC_FILES} )

set_target_properties ( Types PROPERTIES OUTPUT_NAME MantidTypes
                                          COMPILE_DEFINITIONS "Types_EXPORTS;PSAPI_VERSION=1" )
                                          
set_property ( TARGET Types PROPERTY FOLDER "MantidFramework" )
                                          
target_link_libraries(Types LINK_PUBLIC ${Boost_LIBRARIES})
target_include_directories(Types PUBLIC ${CMAKE_BINARY_DIR}/Framework/Types)
#Auto-generate exports header
generate_mantid_export_header(Types)


###########################################################################
# Installation settings
###########################################################################

install ( TARGETS Types ${SYSTEM_PACKAGE_TARGET} DESTINATION ${LIB_DIR} )

# Add the unit tests directory
add_subdirectory ( test )
