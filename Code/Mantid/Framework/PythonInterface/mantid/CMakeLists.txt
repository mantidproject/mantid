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
FUNCTION( SET_PYTHON_PROPERTIES TARGET TARGET_NAME )
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

####################################################################################
# Root module 
####################################################################################
set ( OUTPUT_DIR ${PYTHON_PKG_ROOT} )

set ( PY_FILES
  __init__.py
  simpleapi.py
)

# CMake would not let this hang off the PythonInterface target because
# when adding PythonInterface to Framework in the parent directory
# it can't figure out what file to be dependent on as
# add_custom_target doesn't produce any output.
# For that reason these are added as dependencies to the _kernel module
set ( PYTHON_PKGROOT_INSTALL_FILES "" )
foreach ( PY_FILE ${PY_FILES} )
  add_custom_command ( OUTPUT ${OUTPUT_DIR}/${PY_FILE}
                       DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${PY_FILE}
                       COMMAND ${CMAKE_COMMAND} ARGS -E copy_if_different 
                       ${CMAKE_CURRENT_SOURCE_DIR}/${PY_FILE}
                       ${OUTPUT_DIR}/${PY_FILE} 
  )
  set ( PYTHON_PKGROOT_INSTALL_FILES ${PYTHON_PKGROOT_INSTALL_FILES} ${OUTPUT_DIR}/${PY_FILE} )
endforeach ( PY_FILE )

add_custom_target ( PythonModuleFiles DEPENDS ${PYTHON_PKGROOT_INSTALL_FILES} SOURCES ${PY_FILES} )
set_property ( TARGET PythonModuleFiles PROPERTY FOLDER "MantidFramework/Python" )

####################################################################################
# Sub modules 
####################################################################################
# No prefix for any of them as they are loaded by Python not the OS
set ( CMAKE_SHARED_LIBRARY_PREFIX )

add_subdirectory ( kernel )
add_dependencies ( PythonKernelModule PythonModuleFiles ) # Ensure the module files are in place
#add_subdirectory ( geometry )
add_subdirectory ( api )

# Create an overall target
add_custom_target ( PythonInterface DEPENDS PythonKernelModule PythonAPIModule  )
set_property ( TARGET PythonInterface PROPERTY FOLDER "MantidFramework/Python" )
