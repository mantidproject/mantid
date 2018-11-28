set (_module_name Py3Compat )
string(TOLOWER _module_name _module_name_lower)

set ( PY_FILES
  __init__.py
  enum.py
)


#############################################################################################
# Copy over the pure Python files for the module
#############################################################################################
# Set the destination directory
set ( OUTPUT_DIR ${PYTHON_PKG_ROOT}/py3compat )

if(CMAKE_GENERATOR STREQUAL Xcode)
  # Set the output directory for the libraries.
  set ( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PYTHON_PKG_ROOT}/py3compat )
endif()

copy_files_to_dir ( "${PY_FILES}" ${CMAKE_CURRENT_SOURCE_DIR} ${OUTPUT_DIR}
                     PYTHON_INSTALL_FILES )

# Add a named target with the outputs as dependencies
add_custom_target ( PythonPy3CompatModule ALL DEPENDS ${PYTHON_INSTALL_FILES} )

###########################################################################
# Installation settings
###########################################################################
# Pure Python files
install ( FILES ${PY_FILES} DESTINATION ${BIN_DIR}/mantid/py3compat )
