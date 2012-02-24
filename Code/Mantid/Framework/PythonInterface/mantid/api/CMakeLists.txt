#############################################################################################
# _api Python module
#############################################################################################

set ( MODULE_TEMPLATE src/api.cpp.in )

# Files containing export definitions, these are automatically processed
# -- Do NOT sort this list. The order defines the order in which the export
#    definitions occur and some depend on their base classes being exported first -- 
set ( EXPORT_FILES
  src/Exports/IAlgorithm.cpp
  src/Exports/Algorithm.cpp
  src/Exports/AlgorithmFactory.cpp
  src/Exports/AlgorithmManager.cpp
  src/Exports/AnalysisDataService.cpp
  src/Exports/FileProperty.cpp
  src/Exports/FrameworkManager.cpp
  src/Exports/IEventList.cpp
  src/Exports/ISpectrum.cpp
  src/Exports/WorkspaceHistory.cpp
  src/Exports/ExperimentInfo.cpp
  src/Exports/MultipleExperimentInfos.cpp
  src/Exports/Workspace.cpp
  src/Exports/ITableWorkspace.cpp
  src/Exports/IMDWorkspace.cpp
  src/Exports/IMDHistoWorkspace.cpp
  src/Exports/IMDEventWorkspace.cpp
  src/Exports/MatrixWorkspace.cpp
  src/Exports/IEventWorkspace.cpp
  src/Exports/IPeaksWorkspace.cpp
  src/Exports/BinaryOperations.cpp
  src/Exports/WorkspaceGroup.cpp
  src/Exports/Axis.cpp
  src/Exports/IPeak.cpp
  src/Exports/BoxController.cpp
  src/Exports/FileFinder.cpp
  src/Exports/Sample.cpp
  src/Exports/Run.cpp
)  

# Files containing additional helper code that are not related to exporting class/functions
set ( SRC_FILES 
  src/PythonAlgorithm/AlgorithmWrapper.cpp
  src/CloneMatrixWorkspace.cpp
)

set ( INC_FILES 
  ${HEADER_DIR}/api/PythonAlgorithm/AlgorithmWrapper.h
  ${HEADER_DIR}/api/PythonAlgorithm/PythonAlgorithm.h
  ${HEADER_DIR}/api/BinaryOperations.h
  ${HEADER_DIR}/api/CloneMatrixWorkspace.h
)

set ( PY_FILES 
  __init__.py
  _adsimports.py
  _workspaceops.py
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
