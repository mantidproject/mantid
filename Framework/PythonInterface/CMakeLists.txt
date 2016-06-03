###########################################################################
#
# Python API
#
###########################################################################

###########################################################################
# Set local dependencies
###########################################################################
set ( old_boost ${Boost_LIBRARIES} )
set ( Boost_LIBRARIES ) # Empty out the variable after previous use

# We need to find the right boost python version for the given python version. Unfortunately
# there is not standard naming convention across the platforms, for example:
#  - fedora, arch: libboost_python.so (python2), libboost_python3.so (python3)
#  - debian, ubuntu: libboost_python.so (python2), libboost_python-py34.so (python 3.4)
#  - windows: boost_python (python2), ????? (python3)
#  - others?
if ( MSVC )
  find_package ( Boost COMPONENTS python REQUIRED )
else ()
  if ( PYTHON_VERSION_MAJOR GREATER 2 )
    # Try a known set of suffixes plus a user-defined set
    # Define a cache variable to supply additional suffxies. These are tried first
    set ( BOOST_PYTHON_ADDITIONAL_SUFFIXES "" CACHE STRING "Additional suffixes to try when searching for the boost python3 library. These are prepended to the default list" )
    set ( _suffixes ${BOOST_PYTHON_ADDITIONAL_SUFFIXES} ${PYTHON_VERSION_MAJOR} -py${PYTHON_VERSION_MAJOR}${PYTHON_VERSION_MINOR} )
    foreach ( _suffix ${_suffixes})
      find_package ( Boost COMPONENTS python${_suffix} )
      if ( Boost_FOUND )
        break ()
      endif()
    endforeach ()
    if ( NOT Boost_FOUND )
      message ( FATAL_ERROR "Cannot find appropriate boost python version after trying with the "
                "following library suffixes: ${_suffixes}" )
    endif ()
  else ()
    # Assumes that the default version is 2
    find_package ( Boost COMPONENTS python REQUIRED )
  endif ()
endif ()

set ( Boost_LIBRARIES ${Boost_LIBRARIES} ${old_boost})

include_directories ( SYSTEM ${PYTHON_INCLUDE_PATH} )

find_package ( Numpy REQUIRED )
include_directories ( SYSTEM ${PYTHON_NUMPY_INCLUDE_DIR} )
set ( HEADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/inc/MantidPythonInterface )
include_directories ( inc )
add_definitions ( -DBOOST_PYTHON_NO_LIB -DNPY_NO_DEPRECATED_API=NPY_1_7_API_VERSION )
set ( PYTHON_DEPS ${MPI_CXX_LIBRARIES} )

####################################################################################
# A function for generating the exports
#  - MODULE_TEMPLATE: The file containing the @EXPORT_FUNCTIONS@ and @EXPORT_DECLARE@ flags to replace
#  - OUTPUT_FILE: The path to the generated output file
#  - ... The list of export files
####################################################################################
function ( CREATE_MODULE MODULE_TEMPLATE OUTPUT_FILE )
  set ( _fwd_declarations )
  set ( _function_calls )
  foreach( _cppfile ${ARGN} )
    # pull out all functions named 'void export...'
    file ( STRINGS ${_cppfile} _definitions REGEX "( *)?void *export.*().*" )
    foreach ( _func_definition ${_definitions} )
      # create a forward declaration and function call
      string ( STRIP "${_func_definition}" _func_definition )
      string ( REGEX REPLACE "(void *export.*\\(\\)).*" "\\1" _func_declaration "${_func_definition}" )
      # add to list of declarations
      set ( _fwd_declarations "${_fwd_declarations}\n${_func_declaration}\;" )
      # strip void and add to call list
      string( REGEX REPLACE "void *" "" _func_call "${_func_declaration}" )
      set ( _function_calls "${_function_calls}\n${_func_call}\;" )
    endforeach ()
  endforeach ()
  string ( STRIP "${_fwd_declarations}" _fwd_declarations )
  string ( STRIP "${_function_calls}" _function_calls )

  # Configure the final file
  set ( EXPORT_DECLARE ${_fwd_declarations} )
  set ( EXPORT_FUNCTIONS ${_function_calls} )

  configure_file( ${MODULE_TEMPLATE} ${OUTPUT_FILE} )
endfunction ()

####################################################################################
# A function for setting the correct properties on the individual targets
####################################################################################
function ( SET_PYTHON_PROPERTIES TARGET TARGET_NAME )
  # No library prefixes
  set_target_properties( ${TARGET} PROPERTIES PREFIX "" )
  # Library name needs to end in .pyd for Windows
  if ( MSVC )
    set_target_properties ( ${TARGET} PROPERTIES SUFFIX .pyd
                            COMPILE_FLAGS "/bigobj /w44005 /w44244") # bigobj required for intensive templating
  elseif ( APPLE )
    # and in .so on the Mac
    set_target_properties ( ${TARGET} PROPERTIES SUFFIX .so )
  endif ()
  # Set the name
  set_target_properties ( ${TARGET} PROPERTIES OUTPUT_NAME ${TARGET_NAME} )
  # Debug python library expects imported module names to end in _d
  if ( PYTHON_DEBUG_LIBRARY )
    set_target_properties ( ${TARGET} PROPERTIES DEBUG_OUTPUT_NAME ${TARGET_NAME}_d )
  endif ()
  # Group within VS
  set_property ( TARGET ${TARGET} PROPERTY FOLDER "MantidFramework/Python" )
endfunction()

###########################################################################
# mantid package
###########################################################################
add_subdirectory ( mantid )

###########################################################################
# Python algorithms
###########################################################################
clean_orphaned_pyc_files ( ${CMAKE_CURRENT_SOURCE_DIR}/plugins )

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
