###########################################################################
#
# Python API
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
add_definitions ( -DBOOST_DEBUG_PYTHON -DBOOST_PYTHON_NO_LIB )

if ( UNIX )
  # On some Linux systems (seen on various Ubuntu versions) importing Mantid into a standalone python
  # interpreter causes a segfault. It is some issue due to exception handling but the fix is 
  # to ensure that the stdc++ library appears as early in the link list as possible so that it
  # is loaded first, hence the hard coding of it here rather than leaving it to be implicitly defined
  # by the linker.
  # 
  # MG 2011/11/15: A similar issue regarding the Nexus library has now been observed. Nexus 4.3 introduced
  # thread-local variables and this now causes problems whenever we try and load the library from
  # python if other libraries appear before it in the link list.
  set ( PYTHON_DEPS stdc++ ${NEXUS_C_LIBRARIES} ${MANTIDLIBS} ${Boost_LIBRARIES} ${PYTHON_LIBRARIES}  )
else ()
  set ( PYTHON_DEPS ${MANTIDLIBS} ${Boost_LIBRARIES} ${PYTHON_LIBRARIES}  )
endif ()

####################################################################################
# A macro for generating the exports
#  - MODULE_TEMPLATE: The file containing the @EXPORT_FUNCTIONS@ and @EXPORT_DECALRE@ flags to replace
#  - OUTPUT_FILE: The path to the generated output file
#  - EXPORT_FILES: The variable containing the files to be processed
#  - SRC_FILES: The variable containing the list of sources. Used to append the generated file.    
####################################################################################
MACRO( CREATE_MODULE MODULE_TEMPLATE OUTPUT_FILE EXPORT_FILES SRCS )
  set ( EXPORT_DECLARE )
  set ( EXPORT_FUNCTIONS )
  foreach( CPPFILE ${${EXPORT_FILES}} )
    file( STRINGS ${CPPFILE} EXPORTS REGEX "void *export.*()" )
    foreach( EXPORT ${EXPORTS} )
      string ( STRIP "${EXPORT}" EXPORT )
      set ( EXPORT_DECLARE "${EXPORT_DECLARE}\n${EXPORT};" )
      string( REGEX REPLACE "void " "" EXPORT "${EXPORT}" )
      set ( EXPORT_FUNCTIONS "${EXPORT_FUNCTIONS}\n${EXPORT};" )
    endforeach(EXPORT ${EXPORTS}) 
  endforeach( CPPFILE ${${EXPORT_FILES}} )
  string( STRIP "${EXPORT_DECLARE}" EXPORT_DECLARE )
  string( STRIP "${EXPORT_FUNCTIONS}" EXPORT_FUNCTIONS )
  
  # Configure the final file
  configure_file( ${MODULE_TEMPLATE} ${OUTPUT_FILE} )
  # Set the sources
  LIST ( APPEND ${SRCS} ${${EXPORT_FILES}} ${OUTPUT_FILE} )
ENDMACRO( )

####################################################################################
# A function for setting the correct properties on the individual targets
####################################################################################
FUNCTION( SET_PYTHON_PROPERTIES TARGET TARGET_NAME )
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
ENDFUNCTION() 

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
