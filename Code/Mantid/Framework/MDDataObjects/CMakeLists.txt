# Source files
set (SRC_FILES src/IMD_FileFormat.cpp
src/MDDataPoint.cpp
src/MDDataPointDescription.cpp
src/MDDataPoints.cpp
src/MD_FileFormatFactory.cpp
src/MD_File_hdfMatlab.cpp
src/MD_File_hdfMatlab4D.cpp
src/MD_File_hdfV1.cpp
src/MD_FileHoraceReader.cpp
src/MDImage.cpp
src/MDWorkspace.cpp
)

# Header files
set (INC_FILES inc/MDDataObjects/IMD_FileFormat.h
inc/MDDataObjects/MDDataPoint.h
inc/MDDataObjects/MDDataPointDescription.h
inc/MDDataObjects/MDDataPoints.h
inc/MDDataObjects/MD_FileFormatFactory.h
inc/MDDataObjects/MD_File_hdfMatlab.h
inc/MDDataObjects/MD_File_hdfMatlab4D.h
inc/MDDataObjects/MD_File_hdfV1.h
inc/MDDataObjects/MD_FileHoraceReader.h
inc/MDDataObjects/MDImage.h
inc/MDDataObjects/MDImageDatatypes.h
inc/MDDataObjects/MDWorkspace.h
)

# Test files. Other source files required.
set (TEST_FILES 
test/HoraceReaderTest.h
test/MD_FileFactoryTest.h
)

set ( GMOCK_TEST_FILES
test/MDDataPointTest.h
test/MDImageTest.h
test/MDPixelsTest.h
test/MDPointTest.h
test/MDWorkspaceTest.h
)

add_definitions ( -DHDF_1_6 )
add_definitions ( -DIN_MANTID_MDDATAOBJECTS )

# Add the target for this directory
add_library ( MDDataObjects ${SRC_FILES} ${INC_FILES})
# Set the name of the generated library
set_target_properties ( MDDataObjects PROPERTIES OUTPUT_NAME MantidMDDataObjects )
# Add to the 'Framework' group in VS
set_property ( TARGET MDDataObjects PROPERTY FOLDER "Framework" )
# Find hdf5 libraries
if( WIN32 )
#Exact libraries will need to be refined for windows 64/32 bit as they are different in third_party.
set ( HDF5_INCLUDE_DIRS ${CMAKE_INCLUDE_PATH}/hdf5 )
set ( HDF5_LIBRARIES ${CMAKE_LIBRARY_PATH}/hdf5dll.lib ) 
else()
find_package (HDF5 REQUIRED)
endif()

include_directories(${HDF5_INCLUDE_DIRS} inc)
target_link_libraries ( MDDataObjects ${MANTIDLIBS} ${HDF5_LIBRARIES})

# Create test file projects
if ( CXXTEST_FOUND )
  include_directories(${HDF5_INCLUDE_DIRS} inc)
  if ( GMOCK_FOUND AND GTEST_FOUND )
    cxxtest_add_test ( MDDataObjectsTest ${TEST_FILES} ${GMOCK_TEST_FILES} )
    target_link_libraries( MDDataObjectsTest MDDataObjects ${MANTIDLIBS} ${HDF5_LIBRARIES} gmock )
  else ()
    cxxtest_add_test ( MDDataObjectsTest ${TEST_FILES} )
    target_link_libraries( MDDataObjectsTest MDDataObjects ${MANTIDLIBS} ${HDF5_LIBRARIES} )
  endif ()

endif ()

