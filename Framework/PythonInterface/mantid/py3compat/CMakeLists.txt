set(PY_FILES __init__.py enum.py mock.py)

#
# Copy over the pure Python files for the module
#
# Set the destination directory
set(OUTPUT_DIR ${PYTHON_PKG_ROOT}/py3compat)

if(CMAKE_GENERATOR STREQUAL Xcode)
  # Set the output directory for the libraries.
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PYTHON_PKG_ROOT}/py3compat)
endif()

copy_files_to_dir("${PY_FILES}"
                  ${CMAKE_CURRENT_SOURCE_DIR}
                  ${OUTPUT_DIR}
                  PYTHON_INSTALL_FILES)

# Add a named target with the outputs as dependencies
add_custom_target(PythonPy3CompatModule ALL DEPENDS ${PYTHON_INSTALL_FILES})

#
# Installation settings
#
# Pure Python files
install(FILES ${PY_FILES} DESTINATION ${BIN_DIR}/mantid/py3compat)
install(FILES ${PY_FILES} DESTINATION ${WORKBENCH_BIN_DIR}/mantid/py3compat)
