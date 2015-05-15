set( SRC_FILES
  src/AbortRemoteJob.cpp
  src/AbortRemoteJob2.cpp
  src/Authenticate.cpp
  src/Authenticate2.cpp
  src/DownloadRemoteFile.cpp
  src/DownloadRemoteFile2.cpp
  src/QueryAllRemoteJobs.cpp
  src/QueryAllRemoteJobs2.cpp
  src/QueryRemoteFile.cpp
  src/QueryRemoteFile2.cpp
  src/QueryRemoteJob.cpp
  src/QueryRemoteJob2.cpp
  src/SCARFTomoReconstruction.cpp
  src/SimpleJSON.cpp
  src/StartRemoteTransaction.cpp
  src/StartRemoteTransaction2.cpp
  src/StopRemoteTransaction.cpp
  src/StopRemoteTransaction2.cpp
  src/SubmitRemoteJob.cpp
  src/SubmitRemoteJob2.cpp
  src/UploadRemoteFile.cpp
  src/UploadRemoteFile2.cpp
)

set( INC_FILES
  inc/MantidRemoteAlgorithms/AbortRemoteJob.h
  inc/MantidRemoteAlgorithms/AbortRemoteJob2.h
  inc/MantidRemoteAlgorithms/Authenticate.h
  inc/MantidRemoteAlgorithms/Authenticate2.h
  inc/MantidRemoteAlgorithms/DownloadRemoteFile.h
  inc/MantidRemoteAlgorithms/DownloadRemoteFile2.h
  inc/MantidRemoteAlgorithms/QueryAllRemoteJobs.h
  inc/MantidRemoteAlgorithms/QueryAllRemoteJobs2.h
  inc/MantidRemoteAlgorithms/QueryRemoteFile.h
  inc/MantidRemoteAlgorithms/QueryRemoteFile2.h
  inc/MantidRemoteAlgorithms/QueryRemoteJob.h
  inc/MantidRemoteAlgorithms/QueryRemoteJob2.h
  inc/MantidRemoteAlgorithms/SCARFTomoReconstruction.h
  inc/MantidRemoteAlgorithms/SimpleJSON.h
  inc/MantidRemoteAlgorithms/StartRemoteTransaction.h
  inc/MantidRemoteAlgorithms/StartRemoteTransaction2.h
  inc/MantidRemoteAlgorithms/StopRemoteTransaction.h
  inc/MantidRemoteAlgorithms/StopRemoteTransaction2.h
  inc/MantidRemoteAlgorithms/SubmitRemoteJob.h
  inc/MantidRemoteAlgorithms/SubmitRemoteJob2.h
  inc/MantidRemoteAlgorithms/UploadRemoteFile.h
  inc/MantidRemoteAlgorithms/UploadRemoteFile2.h
)

set ( TEST_FILES
  AbortRemoteJobTest.h
  AbortRemoteJob2Test.h
  AuthenticateTest.h
  Authenticate2Test.h
  DownloadRemoteFileTest.h
  DownloadRemoteFile2Test.h
  QueryAllRemoteJobsTest.h
  QueryAllRemoteJobs2Test.h
  QueryRemoteFileTest.h
  QueryRemoteFile2Test.h
  QueryRemoteJobTest.h
  QueryRemoteJob2Test.h
  SCARFTomoReconstructionTest.h
  SimpleJSONTest.h
  StartRemoteTransactionTest.h
  StartRemoteTransaction2Test.h
  StopRemoteTransactionTest.h
  StopRemoteTransaction2Test.h
  SubmitRemoteJobTest.h
  SubmitRemoteJob2Test.h
  UploadRemoteFileTest.h
  UploadRemoteFile2Test.h
)

#set ( TEST_PY_FILES
#
#)
# No tests yet...

# Not for now, remember later if convenient: Add a precompiled header where they are supported
# enable_precompiled_headers ( inc/MantidRemoteAlgorithms/PrecompiledHeader.h SRC_FILES )

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
