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
  src/Exports/IPropertySettings.cpp
  src/Exports/EnabledWhenProperty.cpp
  src/Exports/VisibleWhenProperty.cpp
  src/Exports/PropertyWithValue.cpp
  src/Exports/ArrayProperty.cpp
  src/Exports/Quat.cpp
  src/Exports/V3D.cpp
  src/Exports/VMD.cpp
  src/Exports/StlContainers.cpp
  src/Exports/Logger.cpp
  src/Exports/Unit.cpp
  src/Exports/Units.cpp
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
  src/Exports/UnitLabel.cpp
  src/Exports/DeltaEMode.cpp
  src/Exports/PropertyManager.cpp
  src/Exports/PropertyHistory.cpp
  src/Exports/Memory.cpp
  src/Exports/ProgressBase.cpp
  src/Exports/Material.cpp
  src/Exports/Statistics.cpp
)

set ( SRC_FILES
  src/Converters/CloneToNumpy.cpp
  src/Converters/NDArrayToVector.cpp
  src/Converters/NDArrayTypeIndex.cpp
  src/Converters/PyArrayType.cpp
  src/Converters/PyObjectToMatrix.cpp
  src/Converters/PyObjectToV3D.cpp
  src/Converters/WrapWithNumpy.cpp
  src/Registry/PropertyWithValueFactory.cpp
  src/Registry/SequenceTypeHandler.cpp
  src/Registry/TypeRegistry.cpp
  src/Registry/DowncastRegistry.cpp
  src/Environment/ErrorHandling.cpp
  src/Environment/Threading.cpp
  src/Environment/WrapperHelpers.cpp
)

set ( INC_FILES
  ${HEADER_DIR}/kernel/Converters/CArrayToNDArray.h
  ${HEADER_DIR}/kernel/Converters/MatrixToNDArray.h
  ${HEADER_DIR}/kernel/Converters/NDArrayToVector.h
  ${HEADER_DIR}/kernel/Converters/NDArrayTypeIndex.h
  ${HEADER_DIR}/kernel/Converters/WrapWithNumpy.h
  ${HEADER_DIR}/kernel/Converters/VectorToNDArray.h
  ${HEADER_DIR}/kernel/Converters/PyArrayType.h
  ${HEADER_DIR}/kernel/Converters/PyObjectToMatrix.h
  ${HEADER_DIR}/kernel/Converters/PyObjectToV3D.h
  ${HEADER_DIR}/kernel/Converters/PySequenceToVector.h
  ${HEADER_DIR}/kernel/Environment/CallMethod.h
  ${HEADER_DIR}/kernel/Environment/ErrorHandling.h
  ${HEADER_DIR}/kernel/Environment/Threading.h
  ${HEADER_DIR}/kernel/Environment/WrapperHelpers.h
  ${HEADER_DIR}/kernel/Policies/MatrixToNumpy.h
  ${HEADER_DIR}/kernel/Policies/VectorToNumpy.h
  ${HEADER_DIR}/kernel/Registry/PropertyValueHandler.h
  ${HEADER_DIR}/kernel/Registry/PropertyWithValueFactory.h
  ${HEADER_DIR}/kernel/Registry/SequenceTypeHandler.h
  ${HEADER_DIR}/kernel/Registry/TypedPropertyValueHandler.h
  ${HEADER_DIR}/kernel/Registry/TypeRegistry.h
  ${HEADER_DIR}/kernel/Registry/DowncastRegistry.h
  ${HEADER_DIR}/kernel/DataServiceExporter.h
  ${HEADER_DIR}/kernel/IsNone.h
  ${HEADER_DIR}/kernel/PropertyWithValueExporter.h
  ${HEADER_DIR}/kernel/PythonObjectInstantiator.h
  ${HEADER_DIR}/kernel/StlExportDefinitions.h
  ${HEADER_DIR}/kernel/TrackingInstanceMethod.h
  ${HEADER_DIR}/kernel/TypedValidatorExporter.h
)

set ( PY_FILES
  __init__.py
  _aliases.py
  environment.py
  funcreturns.py
  plugins.py
)

#############################################################################################
# Generate a source file from the export definitions and provided template
#############################################################################################
create_module ( ${MODULE_TEMPLATE} ${CMAKE_CURRENT_BINARY_DIR}/kernel.cpp EXPORT_FILES
                SRC_FILES )

#############################################################################################
# Add rule to copy over the pure Python files for the module
#############################################################################################
set ( OUTPUT_DIR ${PYTHON_PKG_ROOT}/kernel )

if(CMAKE_GENERATOR STREQUAL Xcode)
  # Set the output directory for the libraries.
  set ( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PYTHON_PKG_ROOT}/kernel )
endif()

copy_files_to_dir ( "${PY_FILES}" ${CMAKE_CURRENT_SOURCE_DIR} ${OUTPUT_DIR} 
                     PYTHON_INSTALL_FILES )

#############################################################################################
# Generate the packagesetup module for the build & install package
#############################################################################################
set ( PACKAGESETUP_PY packagesetup )
set ( NEXUSLIB ${NEXUS_C_LIBRARIES} )
if ( WIN32 ) #.lib -> .dll
  string ( REPLACE ".lib" ".dll" NEXUSLIB ${NEXUSLIB} )
endif()

# Build version
configure_file ( ${CMAKE_CURRENT_SOURCE_DIR}/${PACKAGESETUP_PY}.py.in ${CMAKE_CURRENT_BINARY_DIR}/${PACKAGESETUP_PY}.py )
# Copy py to build directory, taking care of multi-config (MSVC) builds
copy_files_to_dir ( ${PACKAGESETUP_PY}.py ${CMAKE_CURRENT_BINARY_DIR} ${OUTPUT_DIR}
                    PYTHON_INSTALL_FILES )

# Package version
if ( WIN32 OR APPLE )
  # NeXus library is in the same place relative to the Python library
  get_filename_component ( NEXUSLIB_FILE ${NEXUSLIB} NAME )
  set ( NEXUSLIB ../../${NEXUSLIB_FILE} )
endif()
configure_file ( ${CMAKE_CURRENT_SOURCE_DIR}/${PACKAGESETUP_PY}.py.in ${CMAKE_CURRENT_BINARY_DIR}/${PACKAGESETUP_PY}.install.py )

#############################################################################################
# Create the target for this directory
#############################################################################################

add_library ( PythonKernelModule ${SRC_FILES} ${INC_FILES} ${PYTHON_INSTALL_FILES} )
set_python_properties( PythonKernelModule _kernel )
set_target_output_directory ( PythonKernelModule ${OUTPUT_DIR} .pyd )
# Add the required dependencies
target_link_libraries ( PythonKernelModule Kernel ${Boost_LIBRARIES} ${PYTHON_LIBRARIES} ${PYTHON_DEPS} )

###########################################################################
# Installation settings
###########################################################################
install ( TARGETS PythonKernelModule ${SYSTEM_PACKAGE_TARGET} DESTINATION
          ${BIN_DIR}/mantid/kernel )

# Pure Python files
install ( FILES ${PY_FILES} DESTINATION ${BIN_DIR}/mantid/kernel )
# packagesetup.py that will overwrite the ones from the built target
install ( FILES ${CMAKE_CURRENT_BINARY_DIR}/${PACKAGESETUP_PY}.install.py DESTINATION
          ${BIN_DIR}/mantid/kernel RENAME ${PACKAGESETUP_PY}.py )
