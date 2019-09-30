# _kernel Python module

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

# Helper code

set(SRC_FILES
    src/Registry/MappingTypeHandler.cpp
    src/Registry/PropertyManagerFactory.cpp
    src/Registry/PropertyWithValueFactory.cpp
    src/Registry/SequenceTypeHandler.cpp
    src/Registry/TypeRegistry.cpp)

set(INC_FILES
    inc/MantidPythonInterface/kernel/Registry/MappingTypeHandler.h
    inc/MantidPythonInterface/kernel/Registry/PropertyValueHandler.h
    inc/MantidPythonInterface/kernel/Registry/PropertyManagerFactory.h
    inc/MantidPythonInterface/kernel/Registry/PropertyWithValueFactory.h
    inc/MantidPythonInterface/kernel/Registry/SequenceTypeHandler.h
    inc/MantidPythonInterface/kernel/Registry/TypedPropertyValueHandler.h
    inc/MantidPythonInterface/kernel/Registry/TypeRegistry.h
    inc/MantidPythonInterface/kernel/Registry/RegisterWorkspacePtrToPython.h)

# Generate the packagesetup module for the build & install package
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
               ${CMAKE_CURRENT_SOURCE_DIR}/${PACKAGESETUP_PY}.py)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/${MPISETUP_PY}.py.in
               ${CMAKE_CURRENT_SOURCE_DIR}/${MPISETUP_PY}.py)

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

# Create the target for this directory
add_library(PythonKernelModule
            ${EXPORT_FILES}
            ${MODULE_DEFINITION}
            ${SRC_FILES}
            ${INC_FILES}
            ${PYTHON_INSTALL_FILES})
set_python_properties(PythonKernelModule _kernel)

target_include_directories(PythonKernelModule PUBLIC inc)

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

# Installation settings

mtd_install_targets(TARGETS
                    PythonKernelModule
                    INSTALL_DIRS
                    ${LIB_DIR}/mantid/kernel
                    ${WORKBENCH_LIB_DIR}/mantid/kernel)

# packagesetup.py that will overwrite the ones from the built target
mtd_install_files(FILES
                  ${CMAKE_CURRENT_BINARY_DIR}/${PACKAGESETUP_PY}.install.py
                  INSTALL_DIRS
                  ${LIB_DIR}/mantid/kernel
                  ${WORKBENCH_LIB_DIR}/mantid/kernel
                  RENAME
                  ${PACKAGESETUP_PY}.py)

mtd_install_files(FILES
                  ${CMAKE_CURRENT_BINARY_DIR}/${MPISETUP_PY}.install.py
                  INSTALL_DIRS
                  ${LIB_DIR}/mantid/kernel
                  ${WORKBENCH_LIB_DIR}/mantid/kernel
                  RENAME
                  ${MPISETUP_PY}.py)
