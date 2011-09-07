set ( SRC_FILES
	src/NeXusException.cpp
	src/NeXusFile.cpp
	src/NeXusStream.cpp
	src/NexusTestHelper.cpp
)

set ( INC_FILES
	inc/MantidNexusCPP/NeXusException.hpp
	inc/MantidNexusCPP/NeXusFile.hpp
	inc/MantidNexusCPP/NeXusStream.hpp
	inc/MantidNexusCPP/NexusTestHelper.h
)

set ( TEST_FILES
	test/NexusAPITest.h
)

# Add the dependency on the nexus library
find_package ( Nexus REQUIRED )
include_directories ( ${NEXUS_INCLUDE_DIR} src)

add_definitions ( -DIN_NEXUSCPP_LIBRARY )

# Add the target for this directory
add_library ( NexusCPP ${SRC_FILES} ${INC_FILES})
# Set the name of the generated library
set_target_properties ( NexusCPP PROPERTIES OUTPUT_NAME MantidNexusCPP 
                                         COMPILE_DEFINITIONS IN_MANTID_NEXUS )
# Add to the 'Framework' group in VS
set_property ( TARGET NexusCPP PROPERTY FOLDER "MantidFramework" )

include_directories ( inc )

target_link_libraries ( NexusCPP ${NEXUS_LIBRARIES} ${POCO_LIBRARIES} Kernel )

if ( CXXTEST_FOUND )
  cxxtest_add_test ( NexusCPPTest ${TEST_FILES} )
  target_link_libraries( NexusCPPTest NexusCPP  )
  add_dependencies ( FrameworkTests NexusCPPTest )
  # Add to the 'FrameworkTests' group in VS
  set_property ( TARGET NexusCPPTest PROPERTY FOLDER "UnitTests" )
endif ()

###########################################################################
# Installation settings
###########################################################################

install ( TARGETS NexusCPPTest DESTINATION ${LIB_DIR} )
