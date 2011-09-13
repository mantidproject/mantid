set ( SRC_FILES
	src/NeXusException.cpp
	src/NeXusFile.cpp
	src/NeXusStream.cpp
)

set ( INC_FILES
	inc/MantidNexusCPP/NeXusException.hpp
	inc/MantidNexusCPP/NeXusFile.hpp
	inc/MantidNexusCPP/NeXusStream.hpp
)

set ( TEST_FILES
)

# Add the dependency on the nexus library
find_package ( Nexus REQUIRED )
include_directories ( ${NEXUS_INCLUDE_DIR} src)

add_definitions ( -DIN_NEXUS_CPP_LIBRARY )

# Add the target for this directory
add_library ( NexusCPP ${SRC_FILES} ${INC_FILES})
# Set the name of the generated library
set_target_properties ( NexusCPP PROPERTIES OUTPUT_NAME MantidNexusCPP 
                                            COMPILE_DEFINITIONS IN_MANTID_NEXUSCPP )
if( MSVC )                                         
  set_target_properties ( NexusCPP PROPERTIES COMPILE_FLAGS "/w44275 /w44251" )
endif(MSVC)
# Add to the 'Framework' group in VS
set_property ( TARGET NexusCPP PROPERTY FOLDER "MantidFramework" )

include_directories ( inc )

target_link_libraries ( NexusCPP ${NEXUS_LIBRARIES} ${POCO_LIBRARIES} )


###########################################################################
# Installation settings
###########################################################################

install ( TARGETS NexusCPP DESTINATION ${LIB_DIR} )
