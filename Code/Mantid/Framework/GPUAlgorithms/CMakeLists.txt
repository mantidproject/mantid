# Look for the OPENCL paths etc
find_package(OpenCL REQUIRED)

set ( SRC_FILES
	src/GPUAlgorithm.cpp
	src/GPUTester.cpp
)

set ( INC_FILES
	inc/MantidGPUAlgorithms/GPUAlgorithm.h
	inc/MantidGPUAlgorithms/GPUTester.h
)

set ( TEST_FILES
	test/GPUAlgorithmTest.h
	test/GPUTesterTest.h
)

set ( KERNEL_FILES
	GPUTester_kernel.cl
)


# Add the target for this directory
add_library ( GPUAlgorithms ${SRC_FILES} ${INC_FILES} )
# Set the name of the generated library
set_target_properties ( GPUAlgorithms PROPERTIES OUTPUT_NAME MantidGPUAlgorithms )
# Add to the 'Framework' group in VS
set_property ( TARGET Algorithms PROPERTY FOLDER "MantidFramework" )

# Add the OPENCL include folder(s) to the includes
include_directories ( inc ${OPENCL_INCLUDE_DIRS}  )

# Link to the necessary OPENCL libs
target_link_libraries ( GPUAlgorithms ${MANTIDLIBS} ${OPENCL_LIBRARIES} )

 
if ( CXXTEST_FOUND )
  include_directories ( ../Kernel/inc ../API/inc)
  cxxtest_add_test ( GPUAlgorithmsTest ${TEST_FILES} )
  target_link_libraries ( GPUAlgorithmsTest Kernel API GPUAlgorithms)
  add_dependencies ( FrameworkTests GPUAlgorithmsTest )
  # Add to the 'FrameworkTests' group in VS
  set_property ( TARGET GPUAlgorithmsTest PROPERTY FOLDER "UnitTests" )
endif ()
 

#==================== Copy OpenCL kernels to the bin folder ============= 

# This needs to be done as a 2-step process for it to work for VS builds
# First the part that turns relative into absolute paths in the properties file
set ( MANTID_ROOT ${CMAKE_SOURCE_DIR}/Code/Mantid )
# This keeps things working for people building just the framework project
if ( ${CMAKE_PROJECT_NAME} STREQUAL "MantidFramework" )
  set ( MANTID_ROOT ${CMAKE_SOURCE_DIR}/.. )
endif ()

## Make the opencl directory (under bin)
#file( MAKE_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}/opencl/" )

## Go through each file in the set and copy them to bin/ folder.
#foreach(f ${KERNEL_FILES})
#	configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/${f}
#    	${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}/opencl/${f}  COPYONLY)
#endforeach(f) 
    
