# _polarization_helpers Python module

set(MODULE_TEMPLATE src/polarization_helpers.cpp.in)

# Files containing export definitions, these are automatically processed -- Do NOT sort this list. The order defines the
# order in which the export definitions occur and some depend on their base classes being exported first --
set(EXPORT_FILES src/Exports/PolarizationCorrectionHelpers.cpp)

set(MODULE_DEFINITION ${CMAKE_CURRENT_BINARY_DIR}/polarization_helpers.cpp)
create_module(${MODULE_TEMPLATE} ${MODULE_DEFINITION} ${EXPORT_FILES})

# Create the target for this directory
add_library(PythonReflHelpersModule ${MODULE_DEFINITION} ${EXPORT_FILES} ${PYTHON_INSTALL_FILES})
add_library(PythonReflectometryHelpersModule ALIAS PythonReflHelpersModule)

set_python_properties(PythonReflHelpersModule _polarization_helpers)

# Add the required dependencies
target_link_libraries(PythonReflHelpersModule PRIVATE Mantid::Algorithms Mantid::PythonInterfaceCore)

# Installation settings
set_target_properties(PythonReflHelpersModule PROPERTIES INSTALL_RPATH "${EXT_INSTALL_RPATH}")
mtd_install_shared_library(
  TARGETS PythonReflHelpersModule DESTINATION ${Python_SITELIB_RELPATH}/mantid/polarization_helpers.cpp
)
