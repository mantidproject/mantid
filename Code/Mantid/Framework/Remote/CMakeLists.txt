set ( SRC_FILES
	src/RemoteJobManager.cpp
	src/RemoteTask.cpp
	src/SimpleJSON.cpp
)

set ( INC_FILES
	inc/MantidRemote/RemoteJob.h
	inc/MantidRemote/RemoteJobManager.h
	inc/MantidRemote/RemoteTask.h
	inc/MantidRemote/SimpleJSON.h
)

set ( TEST_FILES
)


# Add the target for this directory
add_library ( Remote ${SRC_FILES} ${INC_FILES})
# Set the name of the generated library
set_target_properties ( Remote PROPERTIES OUTPUT_NAME MantidRemote )
# Add to the 'Framework' group in VS
set_property ( TARGET Remote PROPERTY FOLDER "MantidFramework" )

IF (${CMAKE_SYSTEM_NAME} MATCHES "Windows" OR ${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
	SET (HDF5_DIR "${CMAKE_MODULE_PATH}")
	#SET (HDF5_CONFIG "${CMAKE_MODULE_PATH}/hdf5-config.cmake" )
	find_package ( HDF5 COMPONENTS HL REQUIRED
				        CONFIGS hdf5-config.cmake )
ELSE()
	find_package ( HDF5 COMPONENTS HL REQUIRED )
ENDIF()

#message (STATUS "HDF5_INCLUDE_DIRS:" ${HDF5_INCLUDE_DIRS})
#message (STATUS "HDF5_LIBRARIES:" ${HDF5_LIBRARIES})

#include_directories ( inc ../Nexus/inc ${HDF5_INCLUDE_DIRS})
#include_directories ( inc ../Nexus/inc)
include_directories ( inc)

#target_link_libraries ( Remote ${MANTIDLIBS} Nexus ${NEXUS_LIBRARIES} ${HDF5_LIBRARIES})
#target_link_libraries ( Remote ${MANTIDLIBS} Nexus)
target_link_libraries ( Remote ${MANTIDLIBS})

# Add the unit tests directory
add_subdirectory ( test )

###########################################################################
# Installation settings
###########################################################################

install ( TARGETS Remote ${SYSTEM_PACKAGE_TARGET} DESTINATION ${PLUGINS_DIR} )
