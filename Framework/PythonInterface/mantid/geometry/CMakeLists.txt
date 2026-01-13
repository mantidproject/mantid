# _geometry Python module

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
    src/Exports/ComponentInfoPythonIterator.cpp
    src/Exports/MeshObject.cpp
    src/Exports/SampleEnvironment.cpp
    src/Exports/Container.cpp
)

# Generate a source file from the export definitions and provided template

set(MODULE_DEFINITION ${CMAKE_CURRENT_BINARY_DIR}/geometry.cpp)
create_module(${MODULE_TEMPLATE} ${MODULE_DEFINITION} ${EXPORT_FILES})

# Helper code

set(SRC_FILES)

set(INC_FILES inc/MantidPythonInterface/geometry/ComponentInfoPythonIterator.h
              inc/MantidPythonInterface/geometry/DetectorInfoPythonIterator.h
)

list(APPEND ALL_FILES ${EXPORT_FILES} ${SRC_FILES})

# Create the target for this directory
add_library(PythonGeometryModule ${ALL_FILES} ${MODULE_DEFINITION} ${INC_FILES} ${PYTHON_INSTALL_FILES})
add_library(Mantid::PythonGeometryModule ALIAS PythonGeometryModule)
set_python_properties(PythonGeometryModule _geometry)

# Add a precompiled header
target_precompile_headers(
  PythonGeometryModule
  PRIVATE
  <boost/python/class.hpp>
  <boost/python/copy_const_reference.hpp>
  <boost/python/def.hpp>
  <boost/python/default_call_policies.hpp>
  <boost/python/dict.hpp>
  <boost/python/enum.hpp>
  <boost/python/make_constructor.hpp>
  <boost/python/make_function.hpp>
  <boost/python/overloads.hpp>
  <boost/python/register_ptr_to_python.hpp>
  <boost/python/return_value_policy.hpp>
)

target_include_directories(
  PythonGeometryModule PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc> $<INSTALL_INTERFACE:include/Mantid>
)
# Add the required dependencies
target_link_libraries(
  PythonGeometryModule
  PUBLIC Mantid::Geometry
  PRIVATE Mantid::PythonInterfaceCore Mantid::PythonKernelModule Mantid::Beamline Mantid::Types
)

# Installation settings
set_target_properties(PythonGeometryModule PROPERTIES INSTALL_RPATH "${EXT_INSTALL_RPATH};${DL_ORIGIN_TAG}/../kernel/")
mtd_install_shared_library(TARGETS PythonGeometryModule DESTINATION ${Python_SITELIB_RELPATH}/mantid/geometry)
