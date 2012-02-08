#############################################################################################
# _geometry Python module
#############################################################################################

set ( MODULE_TEMPLATE src/geometry.cpp.in )

set ( EXPORT_FILES
  src/IComponent.cpp
  src/ICompAssembly.cpp
  src/IObjComponent.cpp
  src/IDetector.cpp
  src/IMDDimension.cpp
  src/Component.cpp
  src/CompAssembly.cpp
  src/ObjComponent.cpp
  src/ObjCompAssembly.cpp
  src/Detector.cpp
  src/DetectorGroup.cpp
  src/Instrument.cpp
  src/UnitCell.cpp
  src/OrientedLattice.cpp
  src/ReferenceFrame.cpp
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

