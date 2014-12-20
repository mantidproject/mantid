set( SRC_FILES
	src/AbortRemoteJob.cpp
	src/Authenticate.cpp
	src/DownloadRemoteFile.cpp
	src/QueryAllRemoteJobs.cpp
	src/QueryRemoteFile.cpp
	src/QueryRemoteJob.cpp
	src/SimpleJSON.cpp
	src/StartRemoteTransaction.cpp
	src/StopRemoteTransaction.cpp
	src/SubmitRemoteJob.cpp
	src/UploadRemoteFile.cpp
)

set( INC_FILES
	inc/MantidRemoteAlgorithms/AbortRemoteJob.h
	inc/MantidRemoteAlgorithms/Authenticate.h
	inc/MantidRemoteAlgorithms/DownloadRemoteFile.h
	inc/MantidRemoteAlgorithms/QueryAllRemoteJobs.h
	inc/MantidRemoteAlgorithms/QueryRemoteJob.h
	inc/MantidRemoteAlgorithms/QueryRemoteFile.h
	inc/MantidRemoteAlgorithms/SimpleJSON.h
	inc/MantidRemoteAlgorithms/StartRemoteTransaction.h
	inc/MantidRemoteAlgorithms/StopRemoteTransaction.h
	inc/MantidRemoteAlgorithms/SubmitRemoteJob.h
	inc/MantidRemoteAlgorithms/UploadRemoteFile.h
)

#set ( TEST_FILES
#
#)
#set ( TEST_PY_FILES
#
#)
# No tests yet...

# Add the target for this directory
add_library ( RemoteAlgorithms ${SRC_FILES} ${INC_FILES})

# Set the name of the generated library
set_target_properties ( RemoteAlgorithms PROPERTIES OUTPUT_NAME MantidRemoteAlgorithms 
                                                    INSTALL_RPATH "@loader_path/../contents/MacOS/")

# Add to the 'Framework' group in VS
set_property ( TARGET RemoteAlgorithms PROPERTY FOLDER "MantidFramework" )

include_directories ( inc )

target_link_libraries ( RemoteAlgorithms ${MANTIDLIBS} ${GSL_LIBRARIES} )

# Add the unit tests directory
#add_subdirectory ( test )  # No tests yet...

###########################################################################
# Installation settings
###########################################################################

install ( TARGETS RemoteAlgorithms ${SYSTEM_PACKAGE_TARGET} DESTINATION ${PLUGINS_DIR} )



