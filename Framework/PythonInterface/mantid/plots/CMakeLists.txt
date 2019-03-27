set(PY_FILES
    __init__.py
    helperfunctions.py
    plotfunctions.py
    plotfunctions3D.py
    utility.py)

add_subdirectory(modest_image)

# Copy over the pure Python files for the module

# Set the destination directory
set(OUTPUT_DIR ${PYTHON_PKG_ROOT}/plots)

if(CMAKE_GENERATOR STREQUAL Xcode)
  # Set the output directory for the libraries.
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PYTHON_PKG_ROOT}/plots)
endif()

# Add a named target with the outputs as dependencies
add_custom_target(PythonPlotsModule ALL DEPENDS ${PYTHON_INSTALL_FILES})

# Installation settings

# Pure Python files
mtd_install_files(FILES
                  ${PY_FILES}
                  INSTALL_DIRS
                  ${WORKBENCH_BIN_DIR}/mantid/plots
                  ${BIN_DIR}/mantid/plots)
