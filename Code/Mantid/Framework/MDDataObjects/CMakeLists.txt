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
src/MD_FileTestDataGenerator.cpp
src/Events/MDEventWorkspace.cpp
src/Events/MDBox.cpp
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
inc/MDDataObjects/MD_FileTestDataGenerator.h
inc/MDDataObjects/Events/MDPoint.h
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
test/MDBoxTest.h  
test/MDWorkspaceTest.h
)

# Add the target for this directory
add_library ( MDDataObjects ${SRC_FILES} ${INC_FILES})
# Set the name of the generated library
set_target_properties ( MDDataObjects PROPERTIES OUTPUT_NAME MantidMDDataObjects 
                                                 COMPILE_DEFINITIONS IN_MANTID_MDDATAOBJECTS )
# Add to the 'Framework' group in VS
set_property ( TARGET MDDataObjects PROPERTY FOLDER "Framework" )
# Find hdf5 libraries
if( WIN32 )
#Exact libraries will need to be refined for windows 64/32 bit as they are different in third_party.
   # this allows studio using dll-s for hdf5 
    add_definitions ( -D_HDF5USEDLL_ )

	set ( HDF5_INCLUDE_DIRS ${CMAKE_INCLUDE_PATH}/hdf5 )
	# separate configuration files h5config.h for hdf5 in 32 and 64 bit versions
	if ( CMAKE_CL_64 )
		set ( HDF5_INCLUDE_DIRS ${HDF5_INCLUDE_DIRS} ${HDF5_INCLUDE_DIRS}/win64 )
	else()
		set ( HDF5_INCLUDE_DIRS ${HDF5_INCLUDE_DIRS} ${HDF5_INCLUDE_DIRS}/win32 )
	endif()
  # release hdf5dll.lib relates to hdf5.1.6 which does not have debug version when hdf5_18dll is release version for hdf5.1.8 and debug
  # version for hdf5.1.8 is hdf5ddll.lib
	set ( HDF5_LIBRARIES ${CMAKE_LIBRARY_PATH}/hdf5_18dll.lib ) 
else()
# unix build currently works wirh hdf1.6 linked against Nexus.
# Some preprocessor command should already be defined to distinguish between 5.1.6 and 5.1.8 
	find_package (HDF5 REQUIRED)
	add_definitions ( -DHDF_1_6 )
endif()

include_directories(${HDF5_INCLUDE_DIRS} inc)
target_link_libraries ( MDDataObjects ${MANTIDLIBS} ${HDF5_LIBRARIES})

# Create test file projects
if ( CXXTEST_FOUND )
  include_directories(${HDF5_INCLUDE_DIRS} inc)
  if ( GMOCK_FOUND AND GTEST_FOUND )
    cxxtest_add_test ( MDDataObjectsTest ${TEST_FILES} ${GMOCK_TEST_FILES} )
    target_link_libraries( MDDataObjectsTest MDDataObjects ${MANTIDLIBS} ${HDF5_LIBRARIES} ${GMOCK_LIBRARIES} )
  else ()
    cxxtest_add_test ( MDDataObjectsTest ${TEST_FILES} )
    target_link_libraries( MDDataObjectsTest MDDataObjects ${MANTIDLIBS} ${HDF5_LIBRARIES} )
  endif ()
  
  add_dependencies ( FrameworkTests MDDataObjectsTest )
  # Add to the 'FrameworkTests' group in VS
  set_property ( TARGET MDDataObjectsTest PROPERTY FOLDER "FrameworkTests" )
endif ()

