#############################################################################################
# _api Python module
#############################################################################################

set ( MODULE_TEMPLATE src/api.cpp.in )

# Files containing export definitions, these are automatically processed
# -- Do NOT sort this list. The order defines the order in which the export
#    definitions occur and some depend on their base classes being exported first -- 
set ( EXPORT_FILES
  src/Algorithm.cpp
  src/AlgorithmManager.cpp
  src/AnalysisDataService.cpp
  src/ExperimentInfo.cpp
  src/FileProperty.cpp
  src/FrameworkManager.cpp
  src/Workspace.cpp
  src/MatrixWorkspace.cpp
)  

# Files containing addtional helper code that do not require processing
set ( SRC_FILES 
  src/AlgorithmWrapper.cpp
)

set ( INC_FILES 
  ${HEADER_DIR}/api/AlgorithmWrapper.h
)

set ( PY_FILES 
  __init__.py
)

# Set the destination directory
set ( OUTPUT_DIR ${PYTHON_PKG_ROOT}/api )

#############################################################################################
# Generate a source file from the export definitions
#############################################################################################
CREATE_MODULE ( ${MODULE_TEMPLATE} ${CMAKE_CURRENT_BINARY_DIR}/api.cpp EXPORT_FILES SRC_FILES )

#############################################################################################
# Copy over the pure Python files for the module
#############################################################################################
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

add_library ( _api ${SRC_FILES} ${INC_FILES} ${PYTHON_INSTALL_FILES} )
set_python_properties( _api )
move_target ( _api ${OUTPUT_DIR} .pyd )

# Add the required dependencies
target_link_libraries ( _api _kernel ${PYTHON_DEPS} )
