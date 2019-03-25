#
# _kernel Python module
#

set(MODULE_TEMPLATE src/kernel.cpp.in)

# Files containing export definitions, these are automatically processed 
# -- Do NOT sort this list. The order defines the order in which the export
# definitions occur and some depend on their base classes being exported first
# --
set(EXPORT_FILES
    src/Exports/ConfigService.cpp
    src/Exports/ConfigObserver.cpp
    src/Exports/ConfigPropertyObserver.cpp
    src/Exports/DataItem.cpp
    src/Exports/IPropertyManager.cpp
    src/Exports/Property.cpp
    src/Exports/IValidator.cpp
    src/Exports/IPropertySettings.cpp
    src/Exports/EnabledWhenProperty.cpp
    src/Exports/ErrorReporter.cpp
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
    src/Exports/LiveListenerInfo.cpp
    src/Exports/NullValidator.cpp
    src/Exports/ListValidator.cpp
    src/Exports/ArrayLengthValidator.cpp
    src/Exports/ArrayBoundedValidator.cpp
    src/Exports/ArrayOrderedPairsValidator.cpp
    src/Exports/MandatoryValidator.cpp
    src/Exports/CompositeValidator.cpp
    src/Exports/LogFilter.cpp
    src/Exports/UnitConversion.cpp
    src/Exports/UnitFactory.cpp
    src/Exports/UnitLabel.cpp
    src/Exports/DeltaEMode.cpp
    src/Exports/PropertyManager.cpp
    src/Exports/PropertyManagerDataService.cpp
    src/Exports/PropertyManagerProperty.cpp
    src/Exports/PropertyHistory.cpp
    src/Exports/Memory.cpp
    src/Exports/ProgressBase.cpp
    src/Exports/Material.cpp
    src/Exports/MaterialBuilder.cpp
    src/Exports/Statistics.cpp
    src/Exports/OptionalBool.cpp
    src/Exports/UsageService.cpp
    src/Exports/Atom.cpp
    src/Exports/StringContainsValidator.cpp
    src/Exports/PropertyFactory.cpp
    src/Exports/RebinParamsValidator.cpp)

set(MODULE_DEFINITION ${CMAKE_CURRENT_BINARY_DIR}/kernel.cpp)
create_module(${MODULE_TEMPLATE} ${MODULE_DEFINITION} ${EXPORT_FILES})

#
# Helper code
#
set(SRC_FILES
    src/Converters/CloneToNumpy.cpp
    src/Converters/DateAndTime.cpp
    src/Converters/NDArrayToVector.cpp
    src/Converters/NumpyFunctions.cpp
    src/Converters/PyObjectToMatrix.cpp
    src/Converters/PyObjectToV3D.cpp
    src/Converters/PyObjectToVMD.cpp
    src/Registry/MappingTypeHandler.cpp
    src/Registry/PropertyManagerFactory.cpp
    src/Registry/PropertyWithValueFactory.cpp
    src/Registry/SequenceTypeHandler.cpp
    src/Registry/TypeRegistry.cpp)

set(INC_FILES
    ${HEADER_DIR}/kernel/Converters/CArrayToNDArray.h
    ${HEADER_DIR}/kernel/Converters/CloneToNumpy.h
    ${HEADER_DIR}/kernel/Converters/ContainerDtype.h
    ${HEADER_DIR}/kernel/Converters/DateAndTime.h
    ${HEADER_DIR}/kernel/Converters/MapToPyDictionary.h
    ${HEADER_DIR}/kernel/Converters/MatrixToNDArray.h
    ${HEADER_DIR}/kernel/Converters/NumpyFunctions.h
    ${HEADER_DIR}/kernel/Converters/NDArrayToVector.h
    ${HEADER_DIR}/kernel/Converters/PyObjectToMatrix.h
    ${HEADER_DIR}/kernel/Converters/PyObjectToV3D.h
    ${HEADER_DIR}/kernel/Converters/PyObjectToVMD.h
    ${HEADER_DIR}/kernel/Converters/PySequenceToVector.h
    ${HEADER_DIR}/kernel/Policies/MatrixToNumpy.h
    ${HEADER_DIR}/kernel/Policies/RemoveConst.h
    ${HEADER_DIR}/kernel/Policies/ToWeakPtr.h
    ${HEADER_DIR}/kernel/Policies/VectorToNumpy.h
    ${HEADER_DIR}/kernel/Registry/MappingTypeHandler.h
    ${HEADER_DIR}/kernel/Registry/PropertyValueHandler.h
    ${HEADER_DIR}/kernel/Registry/PropertyManagerFactory.h
    ${HEADER_DIR}/kernel/Registry/PropertyWithValueFactory.h
    ${HEADER_DIR}/kernel/Registry/SequenceTypeHandler.h
    ${HEADER_DIR}/kernel/Registry/TypedPropertyValueHandler.h
    ${HEADER_DIR}/kernel/Registry/TypeRegistry.h
    ${HEADER_DIR}/kernel/Registry/RegisterWorkspacePtrToPython.h
    ${HEADER_DIR}/kernel/DataServiceExporter.h
    ${HEADER_DIR}/kernel/IsNone.h
    ${HEADER_DIR}/kernel/PropertyWithValueExporter.h
    ${HEADER_DIR}/kernel/PythonObjectInstantiator.h
    ${HEADER_DIR}/kernel/StlExportDefinitions.h
    ${HEADER_DIR}/kernel/TypedValidatorExporter.h)

set(PY_FILES __init__.py _aliases.py environment.py funcinspect.py plugins.py)

#
# Add rule to copy over the pure Python files for the module
#
set(OUTPUT_DIR ${PYTHON_PKG_ROOT}/kernel)

if(CMAKE_GENERATOR STREQUAL Xcode)
  # Set the output directory for the libraries.
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PYTHON_PKG_ROOT}/kernel)
endif()

copy_files_to_dir("${PY_FILES}"
                  ${CMAKE_CURRENT_SOURCE_DIR}
                  ${OUTPUT_DIR}
                  PYTHON_INSTALL_FILES)

#
# Generate the packagesetup module for the build & install package
#
set(PACKAGESETUP_PY packagesetup)
set(NEXUSLIB ${NEXUS_C_LIBRARIES})
if(WIN32) # .lib -> .dll
  string(REPLACE "/lib/"
                 "/bin/"
                 NEXUSLIB
                 ${NEXUSLIB})
  string(REPLACE ".lib"
                 ".dll"
                 NEXUSLIB
                 ${NEXUSLIB})
endif()
set(MPISETUP_PY mpisetup)
if(MPI_EXPERIMENTAL)
  set(IMPORT_MPI_IN_PYTHON True)
else()
  set(IMPORT_MPI_IN_PYTHON False)
endif()

# Build version
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/${PACKAGESETUP_PY}.py.in
               ${CMAKE_CURRENT_BINARY_DIR}/${PACKAGESETUP_PY}.py)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/${MPISETUP_PY}.py.in
               ${CMAKE_CURRENT_BINARY_DIR}/${MPISETUP_PY}.py)
# Copy py to build directory, taking care of multi-config (MSVC) builds
copy_files_to_dir(${PACKAGESETUP_PY}.py
                  ${CMAKE_CURRENT_BINARY_DIR}
                  ${OUTPUT_DIR}
                  PYTHON_INSTALL_FILES)
copy_files_to_dir(${MPISETUP_PY}.py
                  ${CMAKE_CURRENT_BINARY_DIR}
                  ${OUTPUT_DIR}
                  PYTHON_INSTALL_FILES)

# Package version
if(WIN32 OR (APPLE AND ENABLE_MANTIDPLOT))
  # NeXus library is in the same place relative to the Python library
  get_filename_component(NEXUSLIB_FILE ${NEXUSLIB} NAME)
  set(NEXUSLIB ../../${NEXUSLIB_FILE})
endif()
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/${PACKAGESETUP_PY}.py.in
               ${CMAKE_CURRENT_BINARY_DIR}/${PACKAGESETUP_PY}.install.py)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/${MPISETUP_PY}.py.in
               ${CMAKE_CURRENT_BINARY_DIR}/${MPISETUP_PY}.install.py)

#
# Create the target for this directory
#
add_library(PythonKernelModule
            ${EXPORT_FILES}
            ${MODULE_DEFINITION}
            ${SRC_FILES}
            ${INC_FILES}
            ${PYTHON_INSTALL_FILES})
set_python_properties(PythonKernelModule _kernel)
set_target_output_directory(PythonKernelModule ${OUTPUT_DIR} .pyd)
# Add the required dependencies
target_link_libraries(PythonKernelModule
                      PRIVATE ${TCMALLOC_LIBRARIES_LINKTIME}
                              PythonInterfaceCore
                              Types
                              Kernel
                              ${Boost_LIBRARIES}
                              ${PYTHON_LIBRARIES}
                              ${PYTHON_DEPS}
                              ${POCO_LIBRARIES}
                              ${TBB_LIBRARIES}
                              ${TBB_MALLOC_LIBRARIES})

if(OSX_VERSION VERSION_GREATER 10.8)
  set_target_properties(PythonKernelModule
                        PROPERTIES INSTALL_RPATH "@loader_path/../../../MacOS")
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  set_target_properties(PythonKernelModule
                        PROPERTIES INSTALL_RPATH "\$ORIGIN/../../../${LIB_DIR}")
endif()

#
# Installation settings
#
mtd_install_targets(TARGETS
                    PythonKernelModule
                    INSTALL_DIRS
                    ${BIN_DIR}/mantid/kernel
                    ${WORKBENCH_BIN_DIR}/mantid/kernel)

# Pure Python files
mtd_install_files(FILES
                  ${PY_FILES}
                  INSTALL_DIRS
                  ${BIN_DIR}/mantid/kernel
                  ${WORKBENCH_BIN_DIR}/mantid/kernel)

# packagesetup.py that will overwrite the ones from the built target
mtd_install_files(FILES
                  ${CMAKE_CURRENT_BINARY_DIR}/${PACKAGESETUP_PY}.install.py
                  INSTALL_DIRS
                  ${BIN_DIR}/mantid/kernel
                  ${WORKBENCH_BIN_DIR}/mantid/kernel
                  RENAME
                  ${PACKAGESETUP_PY}.py)

mtd_install_files(FILES
                  ${CMAKE_CURRENT_BINARY_DIR}/${MPISETUP_PY}.install.py
                  INSTALL_DIRS
                  ${BIN_DIR}/mantid/kernel
                  ${WORKBENCH_BIN_DIR}/mantid/kernel
                  RENAME
                  ${MPISETUP_PY}.py)
