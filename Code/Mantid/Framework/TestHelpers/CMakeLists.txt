set ( SRC_FILES src/WorkspaceCreationHelper.cpp 
		src/ComponentCreationHelper.cpp 
		src/TestChannel.cpp )

set ( INC_FILES inc/MantidTestHelpers/DLLExport.h
		inc/MantidTestHelpers/WorkspaceCreationHelper.h
		inc/MantidTestHelpers/ComponentCreationHelper.h 
		inc/MantidTestHelpers/TestChannel.h )

# Requires DataHandling for a load call
include_directories (../DataHandling/inc)
include_directories ( inc )

# For Windows:
add_definitions ( -DIN_MANTID_TESTHELPERS )

# Add the target for this directory
add_library ( TestHelpers ${SRC_FILES} ${INC_FILES})
# Set the name of the generated library
set_target_properties ( TestHelpers PROPERTIES OUTPUT_NAME MantidTestHelpers )
# Set library dependencies
target_link_libraries ( TestHelpers ${MANTIDLIBS} )
