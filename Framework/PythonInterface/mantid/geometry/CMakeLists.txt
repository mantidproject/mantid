#
# _geometry Python module
#

set(MODULE_TEMPLATE src/geometry.cpp.in)

set(EXPORT_FILES
    src/Exports/BoundingBox.cpp
    src/Exports/IComponent.cpp
    src/Exports/ICompAssembly.cpp
    src/Exports/IObject.cpp
    src/Exports/IObjComponent.cpp
    src/Exports/IDetector.cpp
    src/Exports/IMDDimension.cpp
    src/Exports/Component.cpp
    src/Exports/CompAssembly.cpp
    src/Exports/ObjComponent.cpp
    src/Exports/ObjCompAssembly.cpp
    src/Exports/Detector.cpp
    src/Exports/DetectorGroup.cpp
    src/Exports/GridDetector.cpp
    src/Exports/RectangularDetector.cpp
    src/Exports/Instrument.cpp
    src/Exports/UnitCell.cpp
    src/Exports/MDFrame.cpp
    src/Exports/OrientedLattice.cpp
    src/Exports/ReferenceFrame.cpp
    src/Exports/Goniometer.cpp
    src/Exports/CSGObject.cpp
    src/Exports/PeakShape.cpp
    src/Exports/Group.cpp
    src/Exports/PointGroup.cpp
    src/Exports/PointGroupFactory.cpp
    src/Exports/SpaceGroup.cpp
    src/Exports/SpaceGroupFactory.cpp
    src/Exports/SymmetryElement.cpp
    src/Exports/SymmetryElementFactory.cpp
    src/Exports/SymmetryOperation.cpp
    src/Exports/SymmetryOperationFactory.cpp
    src/Exports/CrystalStructure.cpp
    src/Exports/ReflectionGenerator.cpp
    src/Exports/DetectorInfo.cpp
    src/Exports/DetectorInfoItem.cpp
    src/Exports/DetectorInfoPythonIterator.cpp
    src/Exports/ComponentInfo.cpp
    src/Exports/ComponentInfoItem.cpp
    src/Exports/ComponentInfoPythonIterator.cpp)

#
# Generate a source file from the export definitions and provided template
#
set(MODULE_DEFINITION ${CMAKE_CURRENT_BINARY_DIR}/geometry.cpp)
create_module(${MODULE_TEMPLATE} ${MODULE_DEFINITION} ${EXPORT_FILES})

#
# Helper code
#
set(SRC_FILES)

set(INC_FILES)

set(PY_FILES __init__.py _aliases.py)

#
# Copy over the pure Python files for the module
#
# Set the destination directory
set(OUTPUT_DIR ${PYTHON_PKG_ROOT}/geometry)

if(CMAKE_GENERATOR STREQUAL Xcode)
  # Set the output directory for the libraries.
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PYTHON_PKG_ROOT}/geometry)
endif()

copy_files_to_dir("${PY_FILES}"
                  ${CMAKE_CURRENT_SOURCE_DIR}
                  ${OUTPUT_DIR}
                  PYTHON_INSTALL_FILES)

#
# Create the target for this directory
#

add_library(PythonGeometryModule
            ${EXPORT_FILES}
            ${MODULE_DEFINITION}
            ${SRC_FILES}
            ${INC_FILES}
            ${PYTHON_INSTALL_FILES})
set_python_properties(PythonGeometryModule _geometry)
set_target_output_directory(PythonGeometryModule ${OUTPUT_DIR} .pyd)
# Add the required dependencies
target_link_libraries(PythonGeometryModule
                      PRIVATE ${TCMALLOC_LIBRARIES_LINKTIME}
                              PythonInterfaceCore
                              PythonKernelModule
                              Geometry
                              Beamline
                              Types
                              Kernel
                              ${PYTHON_LIBRARIES}
                              ${POCO_LIBRARIES}
                              ${Boost_LIBRARIES}
                              ${TBB_LIBRARIES}
                              ${TBB_MALLOC_LIBRARIES})

if(OSX_VERSION VERSION_GREATER 10.8)
  set_target_properties(
    PythonGeometryModule
    PROPERTIES INSTALL_RPATH
               "@loader_path/../../../MacOS;@loader_path/../kernel/")
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  set_target_properties(
    PythonGeometryModule
    PROPERTIES INSTALL_RPATH "\$ORIGIN/../../../${LIB_DIR};\$ORIGIN/../kernel/")
endif()

#
# Installation settings
#
mtd_install_targets(TARGETS
                    PythonGeometryModule
                    INSTALL_DIRS
                    ${BIN_DIR}/mantid/geometry
                    ${WORKBENCH_BIN_DIR}/mantid/geometry)

# Pure Python files
mtd_install_files(FILES
                  ${PY_FILES}
                  INSTALL_DIRS
                  ${BIN_DIR}/mantid/geometry
                  ${WORKBENCH_BIN_DIR}/mantid/geometry)
