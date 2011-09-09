####################################################################################
# PythonInterface sub modules 
####################################################################################

# Set the root directory for the package
set ( PYTHON_PKG_ROOT ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}/mantid )

####################################################################################
# Define a macro for generating the exports
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
# Define a function for setting the correct properties on the individual targets
####################################################################################
FUNCTION( SET_PYTHON_PROPERTIES TARGET_NAME )
  # Library name needs to end in .pyd for Windows
  if ( MSVC )
    set_target_properties ( ${TARGET_NAME} PROPERTIES SUFFIX .pyd 
                            COMPILE_FLAGS "/bigobj /w44005 /w44244") # bigobj required for intensive templating
  elseif ( APPLE )
    # and in .so on the Mac
    set_target_properties ( ${TARGET_NAME} PROPERTIES SUFFIX .so )
  endif ()
  # Debug python library expects imported module names to end in _d
  if ( PYTHON_DEBUG_LIBRARY )
    set_target_properties ( ${TARGET_NAME} PROPERTIES DEBUG_OUTPUT_NAME ${TARGET_NAME}_d )
  endif ()
  # Group within VS
  set_property ( TARGET ${TARGET_NAME} PROPERTY FOLDER "MantidFramework/PythonInterface" )
ENDFUNCTION() 

####################################################################################
# Sub modules 
####################################################################################
# No prefix for any of them as they are loaded by Python not the OS
set ( CMAKE_SHARED_LIBRARY_PREFIX )

add_subdirectory ( kernel )
#add_subdirectory ( geometry )
add_subdirectory ( api )

####################################################################################
# Python code   
####################################################################################
set ( OUTPUT_DIR ${PYTHON_PKG_ROOT} )

set ( PY_FILES 
  __init__.py
)

set ( PYTHON_INSTALL_FILES "" )
foreach ( PYFILE ${PY_FILES} )
  add_custom_command ( OUTPUT ${OUTPUT_DIR}/${PYFILE}
                       DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${PYFILE}
                       COMMAND ${CMAKE_COMMAND} ARGS -E copy_if_different 
                       ${CMAKE_CURRENT_SOURCE_DIR}/${PYFILE}
                       ${OUTPUT_DIR}/${PYFILE} 
  )
  set ( PYTHON_INSTALL_FILES ${PYTHON_INSTALL_FILES} ${OUTPUT_DIR}/${PYFILE} )
endforeach ( PYFILE )

# Create a target for non MSVC platforms
if ( MSVC )
  add_custom_target ( mantid DEPENDS _kernel _api ${PYTHON_INSTALL_FILES} )
  set_property ( TARGET mantid PROPERTY FOLDER "MantidFramework/PythonInterface" )
else ()
  add_custom_target ( PythonInterface DEPENDS _kernel _api ${PYTHON_INSTALL_FILES} )
endif( MSVC )
