# _dataobjects Python module

set(MODULE_TEMPLATE src/dataobjects.cpp.in)

# Files containing export definitions, these are automatically processed -- Do
# NOT sort this list. The order defines the order in which the export
# definitions occur and some depend on their base classes being exported first
# --
set(EXPORT_FILES
    src/Exports/EventList.cpp
    src/Exports/EventWorkspace.cpp
    src/Exports/EventWorkspaceProperty.cpp
    src/Exports/Workspace2D.cpp
    src/Exports/RebinnedOutput.cpp
    src/Exports/SpecialWorkspace2D.cpp
    src/Exports/GroupingWorkspace.cpp
    src/Exports/MaskWorkspace.cpp
    src/Exports/MaskWorkspaceProperty.cpp
    src/Exports/OffsetsWorkspace.cpp
    src/Exports/MDEventWorkspace.cpp
    src/Exports/MDHistoWorkspace.cpp
    src/Exports/PeaksWorkspace.cpp
    src/Exports/PeaksWorkspaceProperty.cpp
    src/Exports/TableWorkspace.cpp
    src/Exports/SplittersWorkspace.cpp
    src/Exports/WorkspaceSingleValue.cpp)

set(MODULE_DEFINITION ${CMAKE_CURRENT_BINARY_DIR}/dataobjects.cpp)
create_module(${MODULE_TEMPLATE} ${MODULE_DEFINITION} ${EXPORT_FILES})

# Helper code
set(SRC_FILES)

set(INC_FILES)

set(PY_FILES __init__.py)

# Copy over the pure Python files for the module

# Set the destination directory
set(OUTPUT_DIR ${PYTHON_PKG_ROOT}/dataobjects)

if(CMAKE_GENERATOR STREQUAL Xcode)
  # Set the output directory for the libraries.
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PYTHON_PKG_ROOT}/dataobjects)
endif()

# Create the target for this directory
set(FRAMEWORK_DIR ../../..)
include_directories(${FRAMEWORK_DIR}/DataObjects/inc)

add_library(PythonDataObjectsModule
            ${MODULE_DEFINITION}
            ${EXPORT_FILES}
            ${SRC_FILES}
            ${INC_FILES}
            ${PYTHON_INSTALL_FILES})
set_python_properties(PythonDataObjectsModule _dataobjects)

# Add the required dependencies
target_link_libraries(PythonDataObjectsModule
                      LINK_PRIVATE
                      ${TCMALLOC_LIBRARIES_LINKTIME}
                      PythonInterfaceCore
                      PythonAPIModule
                      PythonGeometryModule
                      PythonKernelModule
                      API
                      DataObjects
                      Types
                      Kernel
                      HistogramData
                      Geometry
                      Indexing
                      ${PYTHON_LIBRARIES}
                      ${POCO_LIBRARIES}
                      ${Boost_LIBRARIES})

if(OSX_VERSION VERSION_GREATER 10.8)
  set_target_properties(
    PythonDataObjectsModule
    PROPERTIES
      INSTALL_RPATH
      "@loader_path/../../../MacOS;@loader_path/../kernel/;@loader_path/../geometry/;@loader_path/../api/"
    )
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  set_target_properties(
    PythonDataObjectsModule
    PROPERTIES
      INSTALL_RPATH
      "\$ORIGIN/../../../${LIB_DIR};\$ORIGIN/../kernel/;\$ORIGIN/../geometry/;\$ORIGIN/../api/"
    )
endif()

# Installation settings
mtd_install_targets(TARGETS
                    PythonDataObjectsModule
                    INSTALL_DIRS
                    ${BIN_DIR}/mantid/dataobjects
                    ${WORKBENCH_BIN_DIR}/mantid/dataobjects)

# Pure Python files
mtd_install_files(FILES
                  ${PY_FILES}
                  INSTALL_DIRS
                  ${BIN_DIR}/mantid/dataobjects
                  ${WORKBENCH_BIN_DIR}/mantid/dataobjects)
