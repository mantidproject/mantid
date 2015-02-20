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
  src/SCARFTomoReconstruction.cpp
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
  inc/MantidRemoteAlgorithms/SCARFTomoReconstruction.h
)

set ( TEST_FILES
  SCARFTomoReconstructionTest.h
)

#set ( TEST_PY_FILES
#
#)
# No tests yet...

# Add a precompiled header where they are supported
enable_precompiled_headers ( inc/MantidRemoteAlgorithms/PrecompiledHeader.h SRC_FILES )
# Add the target for this directory
add_library ( RemoteAlgorithms ${SRC_FILES} ${INC_FILES})

# Set the name of the generated library
set_target_properties ( RemoteAlgorithms PROPERTIES OUTPUT_NAME MantidRemoteAlgorithms )

if (OSX_VERSION VERSION_GREATER 10.8)
  set_target_properties ( RemoteAlgorithms PROPERTIES INSTALL_RPATH "@loader_path/../Contents/MacOS")
endif () 

# Add to the 'Framework' group in VS
set_property ( TARGET RemoteAlgorithms PROPERTY FOLDER "MantidFramework" )

include_directories ( inc )

target_link_libraries ( RemoteAlgorithms ${MANTIDLIBS} ${GSL_LIBRARIES} )

# Add the unit tests directory
add_subdirectory ( test )  # Note: No tests yet for many remote algorithms...

###########################################################################
# Installation settings
###########################################################################

install ( TARGETS RemoteAlgorithms ${SYSTEM_PACKAGE_TARGET} DESTINATION ${PLUGINS_DIR} )
