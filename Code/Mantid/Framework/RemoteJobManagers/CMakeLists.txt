set( SRC_FILES
  src/LSFJobManager.cpp
  src/SCARFLSFJobManager.cpp
)

set( INC_FILES
  inc/MantidRemoteJobManagers/LSFJobManager.h
  inc/MantidRemoteJobManagers/SCARFLSFJobManager.h
)

set ( TEST_FILES
  LSFJobManagerTest.h
  SCARFLSFJobManagerTest.h
)

#set ( TEST_PY_FILES
#
#)
# No Python tests yet...

# Not for now, remember later if convenient: Add a precompiled header where they are supported
# enable_precompiled_headers ( inc/MantidRemoteJobManagers/PrecompiledHeader.h SRC_FILES )

# Add the target for this directory
add_library ( RemoteJobManagers ${SRC_FILES} ${INC_FILES})

# Set the name of the generated library
set_target_properties ( RemoteJobManagers PROPERTIES OUTPUT_NAME MantidRemoteJobManagers )

if (OSX_VERSION VERSION_GREATER 10.8)
  set_target_properties ( RemoteJobManagers PROPERTIES INSTALL_RPATH "@loader_path/../Contents/MacOS")
endif () 

# Add to the 'Framework' group in VS
set_property ( TARGET RemoteJobManagers PROPERTY FOLDER "MantidFramework" )

include_directories ( inc )

target_link_libraries ( RemoteJobManagers ${MANTIDLIBS} ${GSL_LIBRARIES} )

# Add the unit tests directory
add_subdirectory ( test )

###########################################################################
# Installation settings
###########################################################################

install ( TARGETS RemoteJobManagers ${SYSTEM_PACKAGE_TARGET} DESTINATION ${PLUGINS_DIR} )
