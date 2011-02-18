set ( SRC_FILES src/WorkspaceCreationHelper.cpp 
		src/ComponentCreationHelper.cpp )

set ( INC_FILES inc/MantidTestHelpers/DLLExport.h
		inc/MantidTestHelpers/WorkspaceCreationHelper.h
		inc/MantidTestHelpers/ComponentCreationHelper.h )

# Requires DataHandling for a load call
include_directories (../DataHandling/inc)
include_directories ( inc )

# For Windows:
add_definitions ( -DIN_MANTID_TESTHELPERS )

# Add the target for this directory
add_library ( TestHelpers EXCLUDE_FROM_ALL ${SRC_FILES} ${INC_FILES})
# Set the name of the generated library
set_target_properties ( TestHelpers PROPERTIES OUTPUT_NAME MantidTestHelpers )
# Add to the 'FrameworkTests' group in VS
set_property ( TARGET TestHelpers PROPERTY FOLDER "UnitTests" )

# Set library dependencies
# Bring in DataHandling
set ( TMP_MANTIDLIBS ${MANTIDLIBS} DataHandling )
target_link_libraries ( TestHelpers ${TMP_MANTIDLIBS}  )

# Cleanup
set ( TMP_MANTIDLIBS )



#======== TARGET FOR A WIKI PAGE MAKER =================
#add_executable(WikiMaker EXCLUDE_FROM_ALL ${CMAKE_CURRENT_SOURCE_DIR}/src/WikiMaker.cpp )

add_executable(WikiMaker ${CMAKE_CURRENT_SOURCE_DIR}/src/WikiMaker.cpp )
target_link_libraries ( WikiMaker ${MANTIDLIBS} Algorithms DataHandling DataObjects Geometry )
