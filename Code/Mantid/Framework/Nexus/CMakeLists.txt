set ( SRC_FILES 
		src/NexusClasses.cpp
		src/NeXusException.cpp
		src/NeXusFile.cpp
		src/NexusFileIO.cpp
		src/NeXusStream.cpp
		#src/LoadRaw/byte_rel_comp.cpp
		#src/LoadRaw/isisraw2.cpp
		#src/LoadRaw/isisraw.cpp
		#src/LoadRaw/item_struct.cpp
		#src/LoadRaw/vms_convert.cpp
		)


set ( INC_FILES 
		inc/MantidNexus/NexusClasses.h
		inc/MantidNexus/NexusFileIO.h
		)

set ( TEST_FILES 
		 test/NexusAPITest.h
		 )

if(UNITY_BUILD)
  include(UnityBuild)
  enable_unity_build(Nexus SRC_FILES SRC_UNITY_IGNORE_FILES 1000)
endif(UNITY_BUILD)

# Add the dependency on the nexus library
find_package ( Nexus REQUIRED )
include_directories ( ${NEXUS_INCLUDE_DIR} src)

add_definitions ( -DIN_NEXUS_CPP_LIBRARY )

# Add the target for this directory
add_library ( Nexus ${SRC_FILES} ${INC_FILES})
# Set the name of the generated library
set_target_properties ( Nexus PROPERTIES OUTPUT_NAME MantidNexus 
                                         COMPILE_DEFINITIONS IN_MANTID_NEXUS )
# Add to the 'Framework' group in VS
set_property ( TARGET Nexus PROPERTY FOLDER "MantidFramework" )

include_directories ( inc )

target_link_libraries ( Nexus ${MANTIDLIBS} ${NEXUS_LIBRARIES} )

if ( CXXTEST_FOUND )
  include_directories (../DataHandling/inc ../TestHelpers/inc)
  cxxtest_add_test ( NexusTest ${TEST_FILES} )
  target_link_libraries( NexusTest Nexus DataHandling TestHelpers)
  add_dependencies ( FrameworkTests NexusTest )
  # Add to the 'FrameworkTests' group in VS
  set_property ( TARGET NexusTest PROPERTY FOLDER "UnitTests" )
endif ()

###########################################################################
# Installation settings
###########################################################################

install ( TARGETS Nexus DESTINATION ${PLUGINS_DIR} )
