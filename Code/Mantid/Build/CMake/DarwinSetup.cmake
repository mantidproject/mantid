set ( CMAKE_INCLUDE_PATH "${THIRD_PARTY}/include" )
set ( BOOST_INCLUDEDIR "${THIRD_PARTY}/include" )

set ( CMAKE_LIBRARY_PATH "${THIRD_PARTY}/lib/mac" )
set ( BOOST_LIBRARYDIR  "${THIRD_PARTY}/lib/mac" )

# For now force 32 bit compile as those are the libraries we've got
set ( CMAKE_C_FLAGS ${CMAKE_C_FLAGS} -m32 )
set ( CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} -m32 )

set ( CMAKE_INSTALL_NAME_DIR ${CMAKE_LIBRARY_PATH} )