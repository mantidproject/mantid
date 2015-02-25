#############################################################################################
# _api Python module
#############################################################################################

set ( MODULE_TEMPLATE src/api.cpp.in )

# Files containing export definitions, these are automatically processed
# -- Do NOT sort this list. The order defines the order in which the export
#    definitions occur and some depend on their base classes being exported first --
set ( EXPORT_FILES
  src/Exports/IAlgorithm.cpp
  src/Exports/AlgorithmProxy.cpp
  src/Exports/AlgorithmHistory.cpp
  src/Exports/CatalogManager.cpp
  src/Exports/CatalogSession.cpp
  src/Exports/DeprecatedAlgorithmChecker.cpp
  src/Exports/Algorithm.cpp
  src/Exports/DataProcessorAlgorithm.cpp
  src/Exports/AlgorithmFactory.cpp
  src/Exports/AlgorithmManager.cpp
  src/Exports/AnalysisDataService.cpp
  src/Exports/FileProperty.cpp
  src/Exports/MultipleFileProperty.cpp
  src/Exports/FrameworkManager.cpp
  src/Exports/IEventList.cpp
  src/Exports/ISpectrum.cpp
  src/Exports/WorkspaceHistory.cpp
  src/Exports/ExperimentInfo.cpp
  src/Exports/MultipleExperimentInfos.cpp
  src/Exports/Workspace.cpp
  src/Exports/IWorkspaceProperty.cpp
  src/Exports/WorkspaceProperty.cpp
  src/Exports/ITableWorkspace.cpp
  src/Exports/ITableWorkspaceProperty.cpp
  src/Exports/MDGeometry.cpp
  src/Exports/IMDWorkspace.cpp
  src/Exports/IMDWorkspaceProperty.cpp
  src/Exports/IMDHistoWorkspace.cpp
  src/Exports/IMDHistoWorkspaceProperty.cpp
  src/Exports/IMDEventWorkspace.cpp
  src/Exports/IMDEventWorkspaceProperty.cpp
  src/Exports/MatrixWorkspace.cpp
  src/Exports/MatrixWorkspaceProperty.cpp
  src/Exports/IEventWorkspace.cpp
  src/Exports/IEventWorkspaceProperty.cpp
  src/Exports/IPeaksWorkspace.cpp
  src/Exports/IPeaksWorkspaceProperty.cpp
  src/Exports/BinaryOperations.cpp
  src/Exports/WorkspaceGroup.cpp
  src/Exports/WorkspaceGroupProperty.cpp
  src/Exports/WorkspaceValidators.cpp
  src/Exports/InstrumentValidator.cpp
  src/Exports/Axis.cpp
  src/Exports/IPeak.cpp
  src/Exports/BoxController.cpp
  src/Exports/FileFinder.cpp
  src/Exports/Sample.cpp
  src/Exports/ScriptRepository.cpp
  src/Exports/ScriptRepositoryFactory.cpp
  src/Exports/Run.cpp
  src/Exports/WorkspaceFactory.cpp
  src/Exports/IFunction.cpp
  src/Exports/IFunction1D.cpp
  src/Exports/IPeakFunction.cpp
  src/Exports/Jacobian.cpp
  src/Exports/PropertyManagerDataService.cpp
  src/Exports/FunctionFactory.cpp
  src/Exports/Progress.cpp
  src/Exports/FunctionProperty.cpp
  src/Exports/AlgorithmProperty.cpp
)

# Files containing additional helper code that are not related to exporting class/functions
set ( SRC_FILES
  src/Algorithms/RunPythonScript.cpp
  src/FitFunctions/IFunctionAdapter.cpp
  src/FitFunctions/IFunction1DAdapter.cpp
  src/FitFunctions/IPeakFunctionAdapter.cpp
  src/PythonAlgorithm/AlgorithmAdapter.cpp
  src/PythonAlgorithm/DataProcessorAdapter.cpp
  src/CloneMatrixWorkspace.cpp
)

set ( INC_FILES
  ${HEADER_DIR}/api/Algorithms/RunPythonScript.h
  ${HEADER_DIR}/api/FitFunctions/IFunctionAdapter.h
  ${HEADER_DIR}/api/FitFunctions/IFunction1DAdapter.h
  ${HEADER_DIR}/api/FitFunctions/IPeakFunctionAdapter.h
  ${HEADER_DIR}/api/PythonAlgorithm/AlgorithmAdapter.h
  ${HEADER_DIR}/api/PythonAlgorithm/DataProcessorAdapter.h
  ${HEADER_DIR}/api/BinaryOperations.h
  ${HEADER_DIR}/api/CloneMatrixWorkspace.h
  ${HEADER_DIR}/api/WorkspacePropertyExporter.h
)

set ( PY_FILES
  __init__.py
  _adsimports.py
  _aliases.py
  _workspaceops.py
)

#############################################################################################
# Generate a source file from the export definitions
#############################################################################################
create_module ( ${MODULE_TEMPLATE} ${CMAKE_CURRENT_BINARY_DIR}/api.cpp EXPORT_FILES
                SRC_FILES )

#############################################################################################
# Copy over the pure Python files for the module
#############################################################################################
# Set the destination directory
set ( OUTPUT_DIR ${PYTHON_PKG_ROOT}/api )

if(CMAKE_GENERATOR STREQUAL Xcode)
  # Set the output directory for the libraries.
  set ( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PYTHON_PKG_ROOT}/api )
endif()

copy_files_to_dir ( "${PY_FILES}" ${CMAKE_CURRENT_SOURCE_DIR} ${OUTPUT_DIR}
                     PYTHON_INSTALL_FILES )

#############################################################################################
# Create the target for this directory
#############################################################################################

add_library ( PythonAPIModule ${SRC_FILES} ${INC_FILES} ${PYTHON_INSTALL_FILES} )
set_python_properties( PythonAPIModule _api )
set_target_output_directory ( PythonAPIModule ${OUTPUT_DIR} .pyd )

# Add the required dependencies
target_link_libraries ( PythonAPIModule PythonGeometryModule PythonKernelModule API )

if (OSX_VERSION VERSION_GREATER 10.8)
  set_target_properties( PythonAPIModule PROPERTIES INSTALL_RPATH "@loader_path/../../../MacOS;@loader_path/../kernel/;@loader_path/../geometry/")
endif ()
###########################################################################
# Installation settings
###########################################################################
install ( TARGETS PythonAPIModule ${SYSTEM_PACKAGE_TARGET} DESTINATION ${BIN_DIR}/mantid/api )

# Pure Python files
install ( FILES ${PY_FILES} DESTINATION ${BIN_DIR}/mantid/api )
