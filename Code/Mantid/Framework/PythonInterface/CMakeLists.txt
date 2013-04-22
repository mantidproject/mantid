###########################################################################
#
# Python API (version 2)
#
###########################################################################

###########################################################################
# Set local dependencies
###########################################################################
set ( Boost_LIBRARIES ) # Empty out the variable after previous use
set ( Boost_USE_DEBUG_PYTHON TRUE )
find_package ( Boost REQUIRED python )

include_directories ( ${PYTHON_INCLUDE_PATH} )

find_package ( Numpy REQUIRED )
include_directories ( SYSTEM ${PYTHON_NUMPY_INCLUDE_DIR} )
set ( HEADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/inc/MantidPythonInterface )
include_directories ( inc )


if ( UNIX )
  ###########################################################################
  # In order to keep the type registries for the two Python APIs separate
  # we need to have a separate copy in each library. Once the old
  # API is removed this will be deleted and we can link back to the dynamic
  # library. This is only required on Unix like environments
  ###########################################################################
  set ( BOOST_PYTHON_SRC ${CMAKE_CURRENT_SOURCE_DIR}/../PythonAPI/src/boostpython.cpp )
  include_directories ( SYSTEM ../PythonAPI/inc )
  
  add_definitions ( -DBOOST_DEBUG_PYTHON -DBOOST_PYTHON_NO_LIB -DBOOST_PYTHON_STATIC_LIB -DBOOST_PYTHON_SOURCE )

  # On some Linux systems (seen on various Ubuntu versions) importing Mantid into a standalone python
  # interpreter causes a segfault. It is some issue due to exception handling but the fix is 
  # to ensure that the stdc++ library appears as early in the link list as possible so that it
  # is loaded first, hence the hard coding of it here rather than leaving it to be implicitly defined
  # by the linker.
  # 
  # MG 2011/11/15: A similar issue regarding the Nexus library has now been observed. Nexus 4.3 introduced
  # thread-local variables and this now causes problems whenever we try and load the library from
  # python if other libraries appear before it in the link list.
  set ( PYTHON_DEPS stdc++ ${NEXUS_C_LIBRARIES} ${MANTIDLIBS} ${PYTHON_LIBRARIES}  )
else ()
  add_definitions ( -DBOOST_DEBUG_PYTHON -DBOOST_PYTHON_NO_LIB )
  set ( BOOST_PYTHON_SRC ) # Not necessary on Windows
  set ( PYTHON_DEPS ${MANTIDLIBS} ${Boost_LIBRARIES} ${PYTHON_LIBRARIES}  )
endif ()

###########################################################################
# mantid package
###########################################################################
add_subdirectory ( mantid )

###########################################################################
# Python algorithms
###########################################################################
clean_orphaned_pyc_files ( ${CMAKE_CURRENT_SOURCE_DIR}/PythonAlgorithms )

###########################################################################
# tests 
###########################################################################
add_subdirectory( test )

###########################################################################
# Installation settings
###########################################################################

# Python algorithms
install ( DIRECTORY plugins/ DESTINATION ${PLUGINS_DIR}/python
          PATTERN "*.pyc" EXCLUDE
          PATTERN ".svn" EXCLUDE )
