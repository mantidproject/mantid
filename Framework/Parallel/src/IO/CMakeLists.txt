cmake_minimum_required(VERSION 2.6)
project(EventParallelLoading)

set(SRC_FILES
        EventLoaderChild.cpp
        EventsListsShmemManager.cpp
        EventsListsShmemStorage.cpp
        )

set(INC_FILES
        ../../inc/MantidParallel/IO/EventsListsShmemManager.h
        ../../inc/MantidParallel/IO/EventsListsShmemStorage.h
        )


#Add executable for Nexus parallel loader child process
add_executable(EventParallelLoader ${SRC_FILES} ${INC_FILES})
#set the name of generated executable
set_target_properties(EventParallelLoader PROPERTIES OUTPUT_NAME MantidNexusParallelLoader
        COMPILE_DEFINITIONS IN_MANTID_PARALLEL)

target_link_libraries(EventParallelLoader LINK_PRIVATE ${TCMALLOC_LIBRARIES_LINKTIME}
        ${GSL_LIBRARIES} ${MANTIDLIBS} ${HDF5_LIBRARIES})
