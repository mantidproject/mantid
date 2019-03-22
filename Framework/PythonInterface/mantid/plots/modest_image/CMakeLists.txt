set ( PY_FILES
  __init__.py
  modest_image.py
)


#############################################################################################
# Copy over the pure Python files for the module
#############################################################################################
# Set the destination directory
set ( OUTPUT_DIR ${PYTHON_PKG_ROOT}/plots/modest_image )

if(CMAKE_GENERATOR STREQUAL Xcode)
  # Set the output directory for the libraries.
  set ( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PYTHON_PKG_ROOT}/plots/modest_image )
endif()

copy_files_to_dir ( "${PY_FILES}" ${CMAKE_CURRENT_SOURCE_DIR} ${OUTPUT_DIR}
                     PYTHON_INSTALL_FILES )

# Add a named target with the outputs as dependencies
add_custom_target ( PythonPlotsModule ALL DEPENDS ${PYTHON_INSTALL_FILES} )

###########################################################################
# Installation settings
###########################################################################
# Pure Python files
mtd_install_files( FILES ${PY_FILES} INSTALL_DIRS ${WORKBENCH_BIN_DIR}/mantid/plots/modest_image ${BIN_DIR}/mantid/plots/modest_image )
