cmake_minimum_required(VERSION 2.6 FATAL_ERROR)
project(GPUAlgorithms)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR})
 
set(LIBRARY_OUTPUT_PATH lib/${CMAKE_BUILD_TYPE})
set(EXECUTABLE_OUTPUT_PATH bin/${CMAKE_BUILD_TYPE})
 
include(FindOpenCL)
 
find_package(OpenCL REQUIRED)
include_directories(${OPENCL_INCLUDE_DIRS} 
	/home/janik/Code/Mantid/Code/Mantid/Framework/Kernel/inc )
 
set ( SRC_FILES
	HelloCL.cpp 
)

set ( INC_FILES
) 

add_library(
        GPUAlgorithms
        ${SRC_FILES} ${INC_FILES}  )
  
target_link_libraries(
        GPUAlgorithms
        ${OPENCL_LIBRARIES} libshrutil.a     liboclUtil.a 
         )

# ------ HELLO CL Exec -------------------------------------------
add_executable(HelloCL HelloCL.cpp )
target_link_libraries ( HelloCL ${OPENCL_LIBRARIES} 
	/home/janik/Code/Mantid/Code/Mantid/bin/libMantidKernel.so )
