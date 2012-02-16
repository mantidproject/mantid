#############################################################################################
# _geometry Python module
#############################################################################################

set ( MODULE_TEMPLATE src/geometry.cpp.in )

set ( EXPORT_FILES
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
  src/Exports/Instrument.cpp
  src/Exports/UnitCell.cpp
  src/Exports/OrientedLattice.cpp
  src/Exports/ReferenceFrame.cpp
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
CREATE_MODULE ( ${MODULE_TEMPLATE} ${CMAKE_CURRENT_BINARY_DIR}/geometry.cpp EXPORT_FILES SRC_FILES )

#############################################################################################
# Copy over the pure Python files for the module
#############################################################################################
# Set the destination directory
set ( OUTPUT_DIR ${PYTHON_PKG_ROOT}/geometry )

set ( PYTHON_INSTALL_FILES "" )
foreach ( PYFILE ${PY_FILES} )
  add_custom_command ( OUTPUT ${OUTPUT_DIR}/${PYFILE}
             DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${PYFILE}
             COMMAND ${CMAKE_COMMAND} ARGS -E copy_if_different 
             ${CMAKE_CURRENT_SOURCE_DIR}/${PYFILE}
             ${OUTPUT_DIR}/${PYFILE}
  )
  set ( PYTHON_INSTALL_FILES ${PYTHON_INSTALL_FILES} ${OUTPUT_DIR}/${PYFILE} )
endforeach ( PYFILE )

#############################################################################################
# Create the target for this directory
#############################################################################################

add_library ( PythonGeometryModule ${SRC_FILES} ${INC_FILES} ${PYTHON_INSTALL_FILES} )
set_python_properties( PythonGeometryModule _geometry )
set_target_output_directory ( PythonGeometryModule ${OUTPUT_DIR} .pyd )
# Add the required dependencies
target_link_libraries ( PythonGeometryModule PythonKernelModule ${PYTHON_DEPS} )

###########################################################################
# Installation settings
###########################################################################
install ( TARGETS PythonGeometryModule DESTINATION ${BIN_DIR}/mantid/geometry )

