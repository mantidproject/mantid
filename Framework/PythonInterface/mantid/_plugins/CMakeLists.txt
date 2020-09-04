# __curvefitting Python module

set(MODULE_TEMPLATE src/curvefitting.cpp.in)

# Files containing export definitions, these are automatically processed -- Do
# NOT sort this list. The order defines the order in which the export
# definitions occur and some depend on their base classes being exported first
set(EXPORT_FILES src/Exports/ProductFunction.cpp)

set(MODULE_DEFINITION ${CMAKE_CURRENT_BINARY_DIR}/curvefitting.cpp)
create_module(${MODULE_TEMPLATE} ${MODULE_DEFINITION} ${EXPORT_FILES})

# Helper code
set(SRC_FILES)

set(INC_FILES)

#
# Create the target for this directory
#

set(FRAMEWORK_DIR ../../..)
include_directories(${FRAMEWORK_DIR}/CurveFitting/inc)

add_library(
  PythonCurveFittingModule ${MODULE_DEFINITION} ${EXPORT_FILES} ${SRC_FILES}
                           ${INC_FILES} ${PYTHON_INSTALL_FILES}
)
set_python_properties(PythonCurveFittingModule _curvefitting)

# Add the required dependencies
target_link_libraries(
  PythonCurveFittingModule
  LINK_PRIVATE
  PythonInterfaceCore
  PythonAPIModule
  PythonGeometryModule
  PythonKernelModule
  API
  CurveFitting
  Kernel
  HistogramData
  Geometry
  ${POCO_LIBRARIES}
  ${Boost_LIBRARIES}
)

# Installation settings
set_target_properties(PythonCurveFittingModule
    PROPERTIES INSTALL_RPATH "${EXT_INSTALL_RPATH};${DL_ORIGIN_TAG}/../kernel/;${DL_ORIGIN_TAG}/../geometry/;${DL_ORIGIN_TAG}/../api/")
mtd_install_targets(
  TARGETS PythonCurveFittingModule INSTALL_DIRS ${SITE_PACKAGES}/mantid/_plugins
  ${WORKBENCH_SITE_PACKAGES}/mantid/_plugins
)
