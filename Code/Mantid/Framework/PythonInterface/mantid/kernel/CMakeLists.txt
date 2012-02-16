#############################################################################################
# _kernel Python module
#############################################################################################

set ( MODULE_TEMPLATE src/kernel.cpp.in )

set ( EXPORT_FILES
  src/ConfigService.cpp
  src/DataItem.cpp
  src/IPropertyManager.cpp
  src/Property.cpp
  src/PropertyWithValue.cpp
  src/ArrayProperty.cpp
  src/Quat.cpp
  src/V3D.cpp
  src/StlContainers.cpp
  src/Logger.cpp
  src/Unit.cpp
  src/IValidator.cpp
  src/BoundedValidator.cpp
  src/TimeSeriesProperty.cpp
  src/DateAndTime.cpp
)  

set ( SRC_FILES
  src/Converters/NDArrayToVectorConverter.cpp
  src/Registry/SequenceTypeHandler.cpp
  src/Registry/TypeRegistry.cpp
  src/Registry/UpcastRegistry.cpp
  src/PyEnvironment.cpp
  src/NumpyConverters.cpp
  src/VectorDelegate.cpp
  src/WrapperHelpers.cpp
)

set ( INC_FILES 
  ${HEADER_DIR}/kernel/Converters/NDArrayToVectorConverter.h
  ${HEADER_DIR}/kernel/Converters/NDArrayTypeIndex.h
  ${HEADER_DIR}/kernel/Converters/PySequenceToVectorConverter.h
  ${HEADER_DIR}/kernel/Policies/upcast_returned_value.h
  ${HEADER_DIR}/kernel/Registry/PropertyValueHandler.h
  ${HEADER_DIR}/kernel/Registry/RegisterSingleValueHandler.h
  ${HEADER_DIR}/kernel/Registry/SequenceTypeHandler.h
  ${HEADER_DIR}/kernel/Registry/SingleValueTypeHandler.h
  ${HEADER_DIR}/kernel/Registry/TypeRegistry.h  
  ${HEADER_DIR}/kernel/Registry/UpcastRegistry.h
  ${HEADER_DIR}/kernel/PythonObjectInstantiator.h
  ${HEADER_DIR}/kernel/PropertyWithValue.h
  ${HEADER_DIR}/kernel/PyEnvironment.h
  ${HEADER_DIR}/kernel/StlExportDefinitions.h
  ${HEADER_DIR}/kernel/VectorDelegate.h
  ${HEADER_DIR}/kernel/NumpyConverters.h
  ${HEADER_DIR}/kernel/WrapperHelpers.h
)

set ( PY_FILES 
  __init__.py
  dlopen.py
  funcreturns.py
  plugins.py
  validators.py
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

add_library ( PythonKernelModule ${SRC_FILES} ${BOOST_PYTHON_SRC} ${INC_FILES} ${PYTHON_INSTALL_FILES} )
set_python_properties( PythonKernelModule _kernel )
set_target_output_directory ( PythonKernelModule ${OUTPUT_DIR} .pyd )
# Add the required dependencies
target_link_libraries ( PythonKernelModule ${PYTHON_DEPS} )

if ( UNIX )
  add_library ( _dlopen src/dlopen.c )
  set_target_properties ( _dlopen PROPERTIES OUTPUT_NAME _dlopen )
  set_target_output_directory ( _dlopen ${OUTPUT_DIR} .pyd )
if ( APPLE )
  # and in .so on the Mac
  # Need to remove OpenMP
  set ( CMAKE_C_FLAGS -m64 )
  set_target_properties ( _dlopen PROPERTIES SUFFIX .so )
  target_link_libraries ( _dlopen ${PYTHON_LIBRARIES} )
endif()
  add_dependencies( PythonKernelModule _dlopen )
endif()


###########################################################################
# Installation settings
###########################################################################
install ( TARGETS PythonKernelModule DESTINATION ${BIN_DIR}/mantid/kernel )
if ( UNIX )
install ( TARGETS _dlopen DESTINATION ${BIN_DIR}/mantid/kernel )
endif()

