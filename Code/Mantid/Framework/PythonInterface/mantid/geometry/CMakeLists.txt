#############################################################################################
# _geometry Python module
#############################################################################################

set ( MODULE_TEMPLATE src/geometry.cpp.in )

set ( EXPORT_FILES
  src/Exports/BoundingBox.cpp
  src/Exports/IComponent.cpp
  src/Exports/ICompAssembly.cpp
  src/Exports/IObjComponent.cpp
  src/Exports/IDetector.cpp
  src/Exports/IMDDimension.cpp
  src/Exports/Component.cpp
  src/Exports/CompAssembly.cpp
  src/Exports/ObjComponent.cpp
  src/Exports/ObjCompAssembly.cpp
  src/Exports/Detector.cpp
  src/Exports/DetectorGroup.cpp
  src/Exports/RectangularDetector.cpp
  src/Exports/Instrument.cpp
  src/Exports/UnitCell.cpp
  src/Exports/OrientedLattice.cpp
  src/Exports/ReferenceFrame.cpp
  src/Exports/Goniometer.cpp
  src/Exports/Object.cpp
  src/Exports/PeakShape.cpp
  src/Exports/PointGroup.cpp
  src/Exports/PointGroupFactory.cpp
  src/Exports/SpaceGroup.cpp
  src/Exports/SpaceGroupFactory.cpp
  src/Exports/SymmetryOperation.cpp
  src/Exports/SymmetryOperationFactory.cpp
)

set ( SRC_FILES
)

set ( INC_FILES
)

set ( PY_FILES
  __init__.py
)

#############################################################################################
# Generate a source file from the export definitions and provided template
#############################################################################################
create_module ( ${MODULE_TEMPLATE} ${CMAKE_CURRENT_BINARY_DIR}/geometry.cpp EXPORT_FILES
                SRC_FILES )

#############################################################################################
# Copy over the pure Python files for the module
#############################################################################################
# Set the destination directory
set ( OUTPUT_DIR ${PYTHON_PKG_ROOT}/geometry )

if(CMAKE_GENERATOR STREQUAL Xcode)
  # Set the output directory for the libraries.
  set ( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PYTHON_PKG_ROOT}/geometry )
endif()

copy_files_to_dir ( "${PY_FILES}" ${CMAKE_CURRENT_SOURCE_DIR} ${OUTPUT_DIR} 
                     PYTHON_INSTALL_FILES )

#############################################################################################
# Create the target for this directory
#############################################################################################

add_library ( PythonGeometryModule ${SRC_FILES} ${INC_FILES} ${PYTHON_INSTALL_FILES} )
set_python_properties( PythonGeometryModule _geometry )
set_target_output_directory ( PythonGeometryModule ${OUTPUT_DIR} .pyd )
# Add the required dependencies
target_link_libraries ( PythonGeometryModule PythonKernelModule Geometry )

if (OSX_VERSION VERSION_GREATER 10.8)
  set_target_properties ( PythonGeometryModule PROPERTIES INSTALL_RPATH "@loader_path/../../../MacOS;@loader_path/../kernel/")
endif ()

###########################################################################
# Installation settings
###########################################################################
install ( TARGETS PythonGeometryModule ${SYSTEM_PACKAGE_TARGET} DESTINATION ${BIN_DIR}/mantid/geometry )

# Pure Python files
install ( FILES ${PY_FILES} DESTINATION ${BIN_DIR}/mantid/geometry )
