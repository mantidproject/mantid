#############################################################################################
# _kernel Python module
#############################################################################################

set ( MODULE_TEMPLATE src/kernel.cpp.in )

set ( EXPORT_FILES
  src/V3D.cpp
#  Property.cpp
)  

set ( INC_FILES )

set ( PY_FILES 
  __init__.py
  dlopen.py
)

# Set the destination directory
set ( OUTPUT_DIR ${PYTHON_PKG_ROOT}/kernel )

#############################################################################################
# Generate a source file from the export definitions
#############################################################################################
CREATE_MODULE ( ${MODULE_TEMPLATE} ${CMAKE_CURRENT_BINARY_DIR}/kernel.cpp EXPORT_FILES SRC_FILES )

#############################################################################################
# Copy over the pure Python files for the module
#############################################################################################
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

#############################################################################################
# Create the target for this directory
#############################################################################################

add_library ( _kernel ${SRC_FILES} ${INC_FILES} ${PYTHON_INSTALL_FILES} )
# Set the target directory to to be inside the Python package
set_target_properties ( _kernel PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${PYTHON_PKG_ROOT}/kernel )
# No prefix as this is loaded by Python not the OS
set ( CMAKE_SHARED_LIBRARY_PREFIX )
# Debug python library expects imported module names to end in _d
if ( PYTHON_DEBUG_LIBRARY )
  set_target_properties ( _kernel PROPERTIES DEBUG_OUTPUT_NAME _kernel_d )
endif ()
# Add the required dependencies
target_link_libraries ( _kernel ${PYTHON_DEPS} )

