#############################################################################################
# _kernel Python module
#############################################################################################

set ( MODULE_TEMPLATE src/kernel.cpp.in )

# Files containing export definitions, these are automatically processed
# -- Do NOT sort this list. The order defines the order in which the export
#    definitions occur and some depend on their base classes being exported first -- 
set ( EXPORT_FILES
  src/Exports/ConfigService.cpp
  src/Exports/DataItem.cpp
  src/Exports/IPropertyManager.cpp
  src/Exports/Property.cpp
  src/Exports/IValidator.cpp
  src/Exports/PropertyWithValue.cpp
  src/Exports/ArrayProperty.cpp
  src/Exports/Quat.cpp
  src/Exports/V3D.cpp
  src/Exports/StlContainers.cpp
  src/Exports/Logger.cpp
  src/Exports/Unit.cpp
  src/Exports/BoundedValidator.cpp
  src/Exports/TimeSeriesProperty.cpp
  src/Exports/FilteredTimeSeriesProperty.cpp
  src/Exports/DateAndTime.cpp
  src/Exports/InstrumentInfo.cpp
  src/Exports/FacilityInfo.cpp
  src/Exports/NullValidator.cpp
  src/Exports/ListValidator.cpp
  src/Exports/ArrayLengthValidator.cpp
  src/Exports/ArrayBoundedValidator.cpp
  src/Exports/MandatoryValidator.cpp
  src/Exports/CompositeValidator.cpp
  src/Exports/LogFilter.cpp
  src/Exports/UnitConversion.cpp
  src/Exports/UnitFactory.cpp
  src/Exports/DeltaEMode.cpp
)  

set ( SRC_FILES
  src/Converters/CloneToNumpy.cpp
  src/Converters/NDArrayToVector.cpp
  src/Converters/NDArrayTypeIndex.cpp
  src/Converters/PyArrayType.cpp
  src/Converters/PyObjectToMatrix.cpp
  src/Converters/PyObjectToV3D.cpp
  src/Converters/WrapWithNumpy.cpp
  src/Registry/SequenceTypeHandler.cpp
  src/Registry/TypeRegistry.cpp
  src/Registry/UpcastRegistry.cpp
  src/Environment/CallStack.cpp
  src/Environment/CallMethod.cpp
  src/Environment/Threading.cpp
  src/Environment/WrapperHelpers.cpp
)

set ( INC_FILES 
  ${HEADER_DIR}/kernel/Converters/CArrayToNDArray.h
  ${HEADER_DIR}/kernel/Converters/MatrixToNDArray.h
  ${HEADER_DIR}/kernel/Converters/NDArrayToVector.h
  ${HEADER_DIR}/kernel/Converters/NDArrayTypeIndex.h
  ${HEADER_DIR}/kernel/Converters/NumpyWrapMode.h
  ${HEADER_DIR}/kernel/Converters/VectorToNDArray.h
  ${HEADER_DIR}/kernel/Converters/PyArrayType.h
  ${HEADER_DIR}/kernel/Converters/PyObjectToMatrix.h
  ${HEADER_DIR}/kernel/Converters/PyObjectToV3D.h
  ${HEADER_DIR}/kernel/Converters/PySequenceToVector.h
  ${HEADER_DIR}/kernel/Environment/CallStack.h
  ${HEADER_DIR}/kernel/Environment/CallMethod.h
  ${HEADER_DIR}/kernel/Environment/Threading.h
  ${HEADER_DIR}/kernel/Environment/WrapperHelpers.h
  ${HEADER_DIR}/kernel/Policies/MatrixToNumpy.h
  ${HEADER_DIR}/kernel/Policies/VectorToNumpy.h
  ${HEADER_DIR}/kernel/Policies/upcast_returned_value.h
  ${HEADER_DIR}/kernel/Registry/PropertyValueHandler.h
  ${HEADER_DIR}/kernel/Registry/RegisterSingleValueHandler.h
  ${HEADER_DIR}/kernel/Registry/SequenceTypeHandler.h
  ${HEADER_DIR}/kernel/Registry/SingleValueTypeHandler.h
  ${HEADER_DIR}/kernel/Registry/TypedPropertyValueHandler.h
  ${HEADER_DIR}/kernel/Registry/TypeRegistry.h  
  ${HEADER_DIR}/kernel/Registry/UpcastRegistry.h
  ${HEADER_DIR}/kernel/PropertyWithValue.h
  ${HEADER_DIR}/kernel/PythonObjectInstantiator.h
  ${HEADER_DIR}/kernel/SharedPtrToPythonMacro.h
  ${HEADER_DIR}/kernel/StlExportDefinitions.h
  ${HEADER_DIR}/kernel/TypedValidatorExportMacro.h
)

set ( PY_FILES 
  __init__.py
  _aliases.py
  dlopen.py
  funcreturns.py
  plugins.py
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

