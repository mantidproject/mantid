set ( SRC_FILES
   src/example.cpp
   src/FakeISISEventDAE.cpp
)

set ( INC_FILES
  #inc/MantidISISLiveData/DllConfig.h
  inc/MantidISISLiveData/TCPEventStreamDefs.h
  inc/MantidISISLiveData/FakeISISEventDAE.h
)

set ( TEST_FILES
)


# Use a precompiled header where they are supported
#enable_precompiled_headers( inc/MantidISISLiveData/PrecompiledHeader.h SRC_FILES )
# Add the target for this directory
add_library ( ISISLiveData ${SRC_FILES} ${INC_FILES})
# Set the name of the generated library
set_target_properties ( ISISLiveData PROPERTIES OUTPUT_NAME MantidISISLiveData 
                                                COMPILE_DEFINITIONS IN_MANTID_ISISLIVEDATA 
                                                INSTALL_RPATH "@loader_path/../Contents/MacOS/")
# Add to the 'Framework' group in VS
set_property ( TARGET ISISLiveData PROPERTY FOLDER "MantidFramework" )

include_directories ( inc )

target_link_libraries ( ISISLiveData ${MANTIDLIBS} )

# Add the unit tests directory
# add_subdirectory ( test )

###########################################################################
# Installation settings
###########################################################################

install ( TARGETS ISISLiveData ${SYSTEM_PACKAGE_TARGET} DESTINATION ${PLUGINS_DIR} )


