set ( SRC_FILES
  src/SplittingFunctions.cpp
  src/Types.cpp
)

set ( INC_FILES
  inc/MantidMPI/SplittingFunctions.h
  inc/MantidMPI/Types.h
  inc/MantidMPI/Helpers.h
)

set ( TEST_FILES
  SplittingFunctionsTest.h
  HelpersTest.h
)

if (COVERALLS)
    foreach( loop_var ${SRC_FILES} ${INC_FILES})
      set_property(GLOBAL APPEND PROPERTY COVERAGE_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/${loop_var}")
    endforeach(loop_var)
endif()

# Add the target for this directory
add_library ( MPI ${SRC_FILES} ${INC_FILES} )
# Set the name of the generated library
set_target_properties ( MPI PROPERTIES OUTPUT_NAME MantidMPI )
# Add to the 'Framework' group in VS
set_property ( TARGET Kernel PROPERTY FOLDER "MantidFramework" )

include_directories ( inc )

target_link_libraries ( MPI ${MANTIDLIBS} )

# Add the unit tests directory
add_subdirectory ( test )

###########################################################################
# Installation settings
###########################################################################

install ( TARGETS MPI ${SYSTEM_PACKAGE_TARGET} DESTINATION ${PLUGINS_DIR} )
