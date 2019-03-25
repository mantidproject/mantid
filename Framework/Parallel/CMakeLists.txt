set(SRC_FILES
    src/Communicator.cpp
    src/ExecutionMode.cpp
    src/IO/Chunker.cpp
    src/IO/EventLoader.cpp
    src/IO/EventParser.cpp
    src/IO/EventsListsShmemManager.cpp
    src/IO/EventsListsShmemStorage.cpp
    src/IO/MultiProcessEventLoader.cpp
    src/IO/NXEventDataLoader.cpp
    src/IO/EventLoaderChild.cpp
    src/Request.cpp
    src/StorageMode.cpp
    src/ThreadingBackend.cpp
    src/IO/EventLoaderHelpers.cpp)

set(INC_FILES
    inc/MantidParallel/Collectives.h
    inc/MantidParallel/Communicator.h
    inc/MantidParallel/ExecutionMode.h
    inc/MantidParallel/IO/Chunker.h
    inc/MantidParallel/IO/EventDataPartitioner.h
    inc/MantidParallel/IO/EventLoader.h
    inc/MantidParallel/IO/EventLoaderHelpers.h
    inc/MantidParallel/IO/EventParser.h
    inc/MantidParallel/IO/EventsListsShmemManager.h
    inc/MantidParallel/IO/EventsListsShmemStorage.h
    inc/MantidParallel/IO/MultiProcessEventLoader.h
    inc/MantidParallel/IO/NXEventDataLoader.h
    inc/MantidParallel/IO/NXEventDataSource.h
    inc/MantidParallel/IO/PulseTimeGenerator.h
    inc/MantidParallel/Nonblocking.h
    inc/MantidParallel/Request.h
    inc/MantidParallel/Status.h
    inc/MantidParallel/StorageMode.h
    inc/MantidParallel/ThreadingBackend.h)

set(TEST_FILES
    ChunkerTest.h
    CollectivesTest.h
    CommunicatorTest.h
    EventDataPartitionerTest.h
    EventLoaderTest.h
    EventParserTest.h
    ExecutionModeTest.h
    NonblockingTest.h
    ParallelRunnerTest.h
    PulseTimeGeneratorTest.h
    RequestTest.h
    StorageModeTest.h
    ThreadingBackendTest.h)

if(COVERALLS)
  foreach(loop_var ${SRC_FILES} ${INC_FILES})
    set_property(GLOBAL APPEND
                 PROPERTY COVERAGE_SRCS
                          "${CMAKE_CURRENT_SOURCE_DIR}/${loop_var}")
  endforeach(loop_var)
endif()

# Add the target for this directory
add_library(Parallel ${SRC_FILES} ${INC_FILES})
add_dependencies(Parallel EventParallelLoader Kernel)
# Set the name of the generated library
set_target_properties(Parallel
                      PROPERTIES OUTPUT_NAME
                                 MantidParallel
                                 COMPILE_DEFINITIONS
                                 IN_MANTID_PARALLEL)

if(OSX_VERSION VERSION_GREATER 10.8)
  set_target_properties(Parallel
                        PROPERTIES INSTALL_RPATH "@loader_path/../MacOS")
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  set_target_properties(Parallel
                        PROPERTIES INSTALL_RPATH "\$ORIGIN/../${LIB_DIR}")
endif()

# Add to the 'Framework' group in VS
set_property(TARGET Parallel PROPERTY FOLDER "MantidFramework")

target_include_directories(Parallel SYSTEM
                           PRIVATE ${HDF5_INCLUDE_DIRS} ${Boost_INCLUDE_DIRS})
target_link_libraries(Parallel
                      LINK_PRIVATE
                      ${TCMALLOC_LIBRARIES_LINKTIME}
                      ${GSL_LIBRARIES}
                      ${MANTIDLIBS}
                      ${HDF5_LIBRARIES}
                      Kernel)

if(UNIX AND NOT APPLE)
  target_link_libraries(Parallel LINK_PRIVATE rt)
endif()

# Add executable for Nexus parallel loader child process
add_executable(EventParallelLoader ${SRC_FILES} ${INC_FILES})
add_dependencies(EventParallelLoader Kernel)
# set the name of generated executable
set_target_properties(EventParallelLoader
                      PROPERTIES OUTPUT_NAME
                                 MantidNexusParallelLoader
                                 COMPILE_DEFINITIONS
                                 IN_MANTID_PARALLEL)

if(OSX_VERSION VERSION_GREATER 10.8)
  set_target_properties(EventParallelLoader
                        PROPERTIES INSTALL_RPATH "@loader_path/../MacOS")
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  set_target_properties(EventParallelLoader
                        PROPERTIES INSTALL_RPATH "\$ORIGIN/../${LIB_DIR}")
endif()

# Add to the 'Framework' group in VS
set_property(TARGET EventParallelLoader PROPERTY FOLDER "MantidFramework")

target_include_directories(EventParallelLoader SYSTEM
                           PRIVATE ${HDF5_INCLUDE_DIRS} ${Boost_INCLUDE_DIRS})

target_link_libraries(EventParallelLoader
                      LINK_PRIVATE
                      ${TCMALLOC_LIBRARIES_LINKTIME}
                      ${GSL_LIBRARIES}
                      ${MANTIDLIBS}
                      ${HDF5_LIBRARIES}
                      Kernel)

if(UNIX AND NOT APPLE)
  target_link_libraries(EventParallelLoader LINK_PRIVATE rt)
endif()

# Add the unit tests directory
add_subdirectory(test)

#
# Installation settings
#

mtd_install_targets(TARGETS
                    Parallel
                    INSTALL_DIRS
                    ${LIB_DIR}
                    ${WORKBENCH_LIB_DIR})
mtd_install_targets(TARGETS
                    EventParallelLoader
                    INSTALL_DIRS
                    ${LIB_DIR}
                    ${WORKBENCH_LIB_DIR})
