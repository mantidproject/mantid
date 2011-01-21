add_definitions ( -DWIN32 -D_WINDOWS -DMS_VISUAL_STUDIO )
add_definitions ( -D_USE_MATH_DEFINES -DNOMINMAX )
add_definitions ( -DGSL_DLL )

set ( CMAKE_INCLUDE_PATH "${THIRD_PARTY}/include" )
set ( BOOST_INCLUDEDIR "${THIRD_PARTY}/include" )
# Multiprocessor compilation
set ( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP" )

set (Boost_NO_SYSTEM_PATHS TRUE)

if ( CMAKE_CL_64 )
  message ( STATUS "64 bit compiler found" )
  set ( CMAKE_LIBRARY_PATH "${THIRD_PARTY}/lib/win64" )
  set ( BOOST_LIBRARYDIR "${THIRD_PARTY}/lib/win64" )
else()
  message ( STATUS "32 bit compiler found" )
  set ( CMAKE_LIBRARY_PATH "${THIRD_PARTY}/lib/win32" )
  set ( BOOST_LIBRARYDIR  "${THIRD_PARTY}/lib/win32" )
endif()