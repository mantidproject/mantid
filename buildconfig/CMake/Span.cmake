include(ExternalProject)

message(STATUS "Using tcbrindle/span in ExternalProject")

# Download and unpack Eigen at configure time
configure_file(${CMAKE_SOURCE_DIR}/buildconfig/CMake/Span.in ${CMAKE_BINARY_DIR}/extern-span/CMakeLists.txt)

# The OLD behavior for this policy is to ignore the visibility properties
# for static libraries, object libraries, and executables without exports.
cmake_policy(SET CMP0063 "OLD")

execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" -DCMAKE_SYSTEM_VERSION=${CMAKE_SYSTEM_VERSION} . WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/extern-span )
execute_process(COMMAND ${CMAKE_COMMAND} --build . WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/extern-span )

set(SPAN_INCLUDE_DIR "${CMAKE_BINARY_DIR}/extern-span/span-prefix/src/span/include" CACHE PATH "")

