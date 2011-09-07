set ( SRC_FILES
        src/MuonNexusReader.cpp
        src/NexusClasses.cpp
        src/NexusFileIO.cpp
)

set ( INC_FILES
        inc/MantidNexus/DllConfig.h
        inc/MantidNexus/MuonNexusReader.h
        inc/MantidNexus/NexusClasses.h
        inc/MantidNexus/NexusFileIO.h
)

set ( TEST_FILES
)

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

# if ( CXXTEST_FOUND )
#  cxxtest_add_test ( NexusTest ${TEST_FILES} )
# target_link_libraries( NexusTest Nexus  )
#  add_dependencies ( FrameworkTests NexusTest )
#  set_property ( TARGET NexusTest PROPERTY FOLDER "UnitTests" )
# endif ()

###########################################################################
# Installation settings
###########################################################################

install ( TARGETS Nexus DESTINATION ${LIB_DIR} )
