#############################################################################################
# _api Python module
#############################################################################################

set ( MODULE_TEMPLATE src/api.cpp.in )

# Files containing export definitions, these are automatically processed
# -- Do NOT sort this list. The order defines the order in which the export
#    definitions occur and some depend on their base classes being exported first -- 
set ( EXPORT_FILES
  src/Algorithm.cpp
  src/AlgorithmFactory.cpp
  src/AlgorithmManager.cpp
  src/AnalysisDataService.cpp
  src/FileProperty.cpp
  src/FrameworkManager.cpp
  src/IEventList.cpp
  src/ISpectrum.cpp
  src/WorkspaceHistory.cpp
  src/ExperimentInfo.cpp
  src/MultipleExperimentInfos.cpp
  src/Workspace.cpp
  src/ITableWorkspace.cpp
  src/IMDWorkspace.cpp
  src/IMDHistoWorkspace.cpp
  src/IMDEventWorkspace.cpp
  src/MatrixWorkspace.cpp
  src/IEventWorkspace.cpp
  src/IPeaksWorkspace.cpp
  src/BinaryOperations.cpp
  src/WorkspaceGroup.cpp
  src/Axis.cpp
  src/IPeak.cpp
  src/BoxController.cpp
  src/FileFinder.cpp
  src/Sample.cpp
  src/Run.cpp
)  

# Files containing additional helper code that are not related to exporting class/functions
set ( SRC_FILES 
  src/AlgorithmWrapper.cpp
  src/WorkspaceToNumpy.cpp
)

set ( INC_FILES 
  ${HEADER_DIR}/api/AlgorithmWrapper.h
  ${HEADER_DIR}/api/BinaryOperations.h
  ${HEADER_DIR}/api/WorkspaceToNumpy.h
)

set ( PY_FILES 
  __init__.py
  workspaceops.py
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

add_library ( PythonAPIModule ${SRC_FILES} ${INC_FILES} ${PYTHON_INSTALL_FILES} )
set_python_properties( PythonAPIModule _api )
set_target_output_directory ( PythonAPIModule ${OUTPUT_DIR} .pyd )

# Add the required dependencies
target_link_libraries ( PythonAPIModule PythonGeometryModule PythonKernelModule ${PYTHON_DEPS} )

###########################################################################
# Installation settings
###########################################################################
install ( TARGETS PythonAPIModule DESTINATION ${BIN_DIR}/mantid/api )
