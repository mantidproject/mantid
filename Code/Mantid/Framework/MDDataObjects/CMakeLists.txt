# GLOBs should be replaced with explicit listings
set ( GLOB SRC_FILES src/*.cpp )

set ( GLOB INC_FILES inc/*.h )

# For Windows:
add_definitions ( -DIN_MANTID_MDDATAOBJECTS )

# Add the target for this directory
add_library ( MDDataObjects ${SRC_FILES} ${INC_FILES})
# Set the name of the generated library
set_target_properties ( MDDataObjects PROPERTIES OUTPUT_NAME MantidMDDataObjects )
# Add to the 'Framework' group in VS
set_property ( TARGET MDDataObjects PROPERTY FOLDER "Framework" )

include_directories ( inc )

target_link_libraries ( MDDataObjects ${MANTIDLIBS} )