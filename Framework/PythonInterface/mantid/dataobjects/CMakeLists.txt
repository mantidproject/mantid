# _dataobjects Python module

set(MODULE_TEMPLATE src/dataobjects.cpp.in)

# Files containing export definitions, these are automatically processed -- Do NOT sort this list. The order defines the
# order in which the export definitions occur and some depend on their base classes being exported first --
set(EXPORT_FILES
    src/Exports/EventList.cpp
    src/Exports/EventWorkspace.cpp
    src/Exports/EventWorkspaceProperty.cpp
    src/Exports/Workspace2D.cpp
    src/Exports/Workspace2DProperty.cpp
    src/Exports/RebinnedOutput.cpp
    src/Exports/SpecialWorkspace2D.cpp
    src/Exports/SpecialWorkspace2DProperty.cpp
    src/Exports/GroupingWorkspace.cpp
    src/Exports/GroupingWorkspaceProperty.cpp
    src/Exports/MaskWorkspace.cpp
    src/Exports/MaskWorkspaceProperty.cpp
    src/Exports/OffsetsWorkspace.cpp
    src/Exports/OffsetsWorkspaceProperty.cpp
    src/Exports/MDEventWorkspace.cpp
    src/Exports/MDHistoWorkspace.cpp
    src/Exports/PeaksWorkspace.cpp
    src/Exports/LeanElasticPeaksWorkspace.cpp
    src/Exports/PeaksWorkspaceProperty.cpp
    src/Exports/TableWorkspace.cpp
    src/Exports/TableWorkspaceProperty.cpp
    src/Exports/SplittersWorkspace.cpp
    src/Exports/SplittersWorkspaceProperty.cpp
    src/Exports/WorkspaceSingleValue.cpp
    src/Exports/WorkspaceSingleValueProperty.cpp
    src/Exports/NoShape.cpp
    src/Exports/PeakShapeSpherical.cpp
    src/Exports/PeakShapeEllipsoid.cpp
    src/Exports/WorkspaceValidators.cpp
)

set(MODULE_DEFINITION ${CMAKE_CURRENT_BINARY_DIR}/dataobjects.cpp)
create_module(${MODULE_TEMPLATE} ${MODULE_DEFINITION} ${EXPORT_FILES})

# Helper code
set(SRC_FILES)

set(INC_FILES)

# Create the target for this directory

add_library(
  PythonDataObjectsModule ${MODULE_DEFINITION} ${EXPORT_FILES} ${SRC_FILES} ${INC_FILES} ${PYTHON_INSTALL_FILES}
)
add_library(Mantid::PythonDataObjectsModule ALIAS PythonDataObjectsModule)

target_include_directories(
  PythonDataObjectsModule PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc> $<INSTALL_INTERFACE:include/Mantid>
)

set_python_properties(PythonDataObjectsModule _dataobjects)

# Add the required dependencies
target_link_libraries(
  PythonDataObjectsModule
  PUBLIC Mantid::DataObjects
  PRIVATE Mantid::PythonInterfaceCore
          Mantid::PythonAPIModule
          Mantid::PythonGeometryModule
          Mantid::PythonKernelModule
          Mantid::Types
          Mantid::HistogramData
          Mantid::Indexing
)

# Installation settings
set_target_properties(
  PythonDataObjectsModule
  PROPERTIES INSTALL_RPATH
             "${EXT_INSTALL_RPATH};${DL_ORIGIN_TAG}/../kernel/;${DL_ORIGIN_TAG}/../geometry/;${DL_ORIGIN_TAG}/../api/"
)
mtd_install_shared_library(TARGETS PythonDataObjectsModule DESTINATION ${Python_SITELIB_RELPATH}/mantid/dataobjects)
