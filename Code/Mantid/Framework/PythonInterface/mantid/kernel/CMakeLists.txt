#############################################################################################
# _kernel Python module
#############################################################################################

set ( MODULE_TEMPLATE src/kernel.cpp.in )

set ( EXPORT_FILES
  src/DataItem.cpp
  src/IPropertyManager.cpp
  src/Property.cpp
  src/PropertyWithValue.cpp
  src/ArrayProperty.cpp
  src/Quat.cpp
  src/V3D.cpp
  src/StlContainers.cpp
)  

set ( SRC_FILES
  src/PropertyMarshal.cpp
  src/SequenceTypeHandler.cpp
  src/VectorDelegate.cpp
  src/WrapperHelpers.cpp
)

set ( INC_FILES 
  ${HEADER_DIR}/kernel/PythonObjectInstantiator.h
  ${HEADER_DIR}/kernel/PropertyWithValue.h
  ${HEADER_DIR}/kernel/PropertyHandler.h
  ${HEADER_DIR}/kernel/PropertyMarshal.h
  ${HEADER_DIR}/kernel/SingleValueTypeHandler.h
  ${HEADER_DIR}/kernel/StlExportDefinitions.h
  ${HEADER_DIR}/kernel/VectorDelegate.h
  ${HEADER_DIR}/kernel/WrapperHelpers.h
)

set ( PY_FILES 
  __init__.py
  dlopen.py
)

#############################################################################################
# Generate a source file from the export definitions and provided template
#############################################################################################
CREATE_MODULE ( ${MODULE_TEMPLATE} ${CMAKE_CURRENT_BINARY_DIR}/kernel.cpp EXPORT_FILES SRC_FILES )

#############################################################################################
# Copy over the pure Python files for the module
#############################################################################################
# Set the destination directory
set ( OUTPUT_DIR ${PYTHON_PKG_ROOT}/kernel )

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

add_library ( PythonKernelModule ${SRC_FILES} ${INC_FILES} ${PYTHON_INSTALL_FILES} )
set_python_properties( PythonKernelModule _kernel )
set_target_output_directory ( PythonKernelModule ${OUTPUT_DIR} .pyd )
# Add the required dependencies
target_link_libraries ( PythonKernelModule ${PYTHON_DEPS} )
