#############################################################################################
# testhelpers Python module
#############################################################################################

set ( PY_FILES
  __init__.py
  algorithm_decorator.py
  tempfile_wrapper.py
)

# Copy python files to output directory
set ( OUTPUT_DIR ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}/testhelpers )
copy_files_to_dir ( "${PY_FILES}" ${CMAKE_CURRENT_SOURCE_DIR} ${OUTPUT_DIR} 
                     PYTHON_INSTALL_FILES )

#############################################################################################
# Create the targets for this directory
#############################################################################################
set ( FRAMEWORK_DIR ../../.. )
include_directories ( ${FRAMEWORK_DIR}/CurveFitting/inc ${FRAMEWORK_DIR}/DataHandling/inc ${FRAMEWORK_DIR}/DataObjects/inc
                      ${FRAMEWORK_DIR}/Nexus/inc ${FRAMEWORK_DIR}/TestHelpers/inc )

# WorkspaceCreationHelper
set ( SRC_FILES
  WorkspaceCreationHelperModule.cpp 
  ${FRAMEWORK_DIR}/TestHelpers/src/ComponentCreationHelper.cpp
  ${FRAMEWORK_DIR}/TestHelpers/src/MDEventsTestHelper.cpp
  ${FRAMEWORK_DIR}/TestHelpers/src/WorkspaceCreationHelper.cpp
)
add_library ( PythonWorkspaceCreationHelper ${SRC_FILES} ${INC_FILES} ${PYTHON_INSTALL_FILES} )
set_python_properties( PythonWorkspaceCreationHelper WorkspaceCreationHelper )
set_target_output_directory ( PythonWorkspaceCreationHelper ${OUTPUT_DIR} .pyd )
# Override folder
set_property ( TARGET PythonWorkspaceCreationHelper PROPERTY FOLDER "UnitTests/Python" )
target_link_libraries ( PythonWorkspaceCreationHelper PythonAPIModule DataObjects DataHandling )

# Overall testhelpers target
add_custom_target ( testhelpers DEPENDS PythonWorkspaceCreationHelper )
add_dependencies ( FrameworkTests testhelpers )
set_property ( TARGET testhelpers PROPERTY FOLDER "UnitTests/Python" )
