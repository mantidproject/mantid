#############################################################################################
# testhelpers Python module
#############################################################################################

set ( PY_FILES
  __init__.py
)

# Copy python files to output directory
set ( OUTPUT_DIR ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}/testhelpers )
copy_python_files_to_dir ( "${PY_FILES}" ${CMAKE_CURRENT_SOURCE_DIR} ${OUTPUT_DIR} 
                           PYTHON_INSTALL_FILES )

#############################################################################################
# Create the targets for this directory
#############################################################################################
set ( FRAMEWORK_DIR ../../.. )
include_directories ( ${FRAMEWORK_DIR}/CurveFitting/inc ${FRAMEWORK_DIR}/DataHandling/inc ${FRAMEWORK_DIR}/DataObjects/inc
                      ${FRAMEWORK_DIR}/Nexus/inc ${FRAMEWORK_DIR}/TestHelpers/inc ${FRAMEWORK_DIR}/MDEvents/inc )

# WorkspaceCreationHelper
add_library ( WorkspaceCreationHelper WorkspaceCreationHelperModule.cpp ${FRAMEWORK_DIR}/TestHelpers/src/WorkspaceCreationHelper.cpp
              ${BOOST_PYTHON_SRC} ${INC_FILES} ${PYTHON_INSTALL_FILES} )
set_python_properties( WorkspaceCreationHelper WorkspaceCreationHelper )
set_target_output_directory ( WorkspaceCreationHelper ${OUTPUT_DIR} .pyd )
