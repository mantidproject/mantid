if ( MPI_EXPERIMENTAL )
  set ( MPI_SRC_FILES
    src/Communicator.cpp
    src/ParallelRunner.cpp
    src/Request.cpp
    src/Status.cpp
    src/ThreadingBackend.cpp
    )

  set ( MPI_INC_FILES
    inc/MantidMPI/Communicator.h
    inc/MantidMPI/ParallelRunner.h
    inc/MantidMPI/Request.h
    inc/MantidMPI/Status.h
    inc/MantidMPI/ThreadingBackend.h
    )

  set ( MPI_TEST_FILES
    CommunicatorTest.h
    ParallelRunnerTest.h
    RequestTest.h
    StatusTest.h
    ThreadingBackendTest.h
    )
endif ()

set ( SRC_FILES
  ${MPI_SRC_FILES}
  src/dummy.cpp
)

set ( INC_FILES
  ${MPI_INC_FILES}
)

set ( TEST_FILES
  ${MPI_TEST_FILES}
)

if (COVERALLS)
  foreach( loop_var ${SRC_FILES} ${INC_FILES})
    set_property(GLOBAL APPEND PROPERTY COVERAGE_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/${loop_var}")
  endforeach(loop_var)
endif()

if(UNITY_BUILD)
  include(UnityBuild)
  enable_unity_build(MPI SRC_FILES SRC_UNITY_IGNORE_FILES 10)
endif(UNITY_BUILD)

# Add the target for this directory
add_library ( MPI ${SRC_FILES} ${INC_FILES} )
# Set the name of the generated library
set_target_properties ( MPI PROPERTIES OUTPUT_NAME MantidMPI
  COMPILE_DEFINITIONS IN_MANTID_MPI )

if (OSX_VERSION VERSION_GREATER 10.8)
  set_target_properties ( MPI PROPERTIES INSTALL_RPATH "@loader_path/../MacOS")
endif ()

# Add to the 'Framework' group in VS
set_property ( TARGET MPI PROPERTY FOLDER "MantidFramework" )

target_link_libraries ( MPI LINK_PRIVATE ${TCMALLOC_LIBRARIES_LINKTIME} 
                        ${GSL_LIBRARIES} ${MANTIDLIBS} )

# Add the unit tests directory
add_subdirectory ( test )

###########################################################################
# Installation settings
###########################################################################

install ( TARGETS MPI ${SYSTEM_PACKAGE_TARGET} DESTINATION ${LIB_DIR} )
