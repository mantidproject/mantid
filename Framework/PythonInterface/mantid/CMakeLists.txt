#
# PythonInterface sub modules
#

# Set the root directory for the package
set(PYTHON_PKG_ROOT
    ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}/mantid)

#
# Root module
#
# The destination directory, used by sub modules
set(OUTPUT_DIR ${PYTHON_PKG_ROOT})

# Select the pure Python files to be included in the distribution
set(PY_FILES __init__.py simpleapi.py fitfunctions.py)

copy_files_to_dir("${PY_FILES}"
                  ${CMAKE_CURRENT_SOURCE_DIR}
                  ${OUTPUT_DIR}
                  PYTHON_PKGROOT_INSTALL_FILES)
# Generate the version module
set(VERSION_PY pyversion)
# Build version
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/${VERSION_PY}.py.in
               ${CMAKE_CURRENT_BINARY_DIR}/${VERSION_PY}.py)
# Copy py to build directory, taking care of multi-config (MSVC) builds
copy_files_to_dir(${VERSION_PY}.py
                  ${CMAKE_CURRENT_BINARY_DIR}
                  ${OUTPUT_DIR}
                  PYTHON_PKGROOT_INSTALL_FILES)
# Package version
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/${VERSION_PY}.py.in
               ${CMAKE_CURRENT_BINARY_DIR}/${VERSION_PY}.install.py)

# There is no library target for the top level package just the python files
add_custom_target(PythonModule
                  DEPENDS ${PYTHON_PKGROOT_INSTALL_FILES} ${PYTHON_EXE_WRAPPER}
                  SOURCES ${PY_FILES})
set_property(TARGET PythonModule PROPERTY FOLDER "MantidFramework/Python")

#
# Sub modules
#
add_subdirectory(kernel)
add_dependencies(PythonKernelModule PythonModule) # Ensure the module files are
                                                  # in place
add_subdirectory(geometry)
add_subdirectory(api)
add_subdirectory(dataobjects)
add_subdirectory(_plugins)
add_subdirectory(plots)
add_subdirectory(py3compat)

# Create an overall target
add_custom_target(PythonInterface
                  DEPENDS PythonInterfaceCore
                          PythonKernelModule
                          PythonGeometryModule
                          PythonAPIModule
                          PythonDataObjectsModule
                          PythonCurveFittingModule
                          PythonPlotsModule
                          PythonPy3CompatModule)
set_property(TARGET PythonInterface PROPERTY FOLDER "MantidFramework/Python")

#
# Bundle Python DLL (where appropriate)
#
# Defines the target the will cause the Python bundle to be copied
set(PYBUNDLE_POST_TARGET PythonModule)
include(${CMAKE_CURRENT_SOURCE_DIR}/BundlePython.cmake)

#
# Installation settings
#

# Pure Python files
mtd_install_files(FILES
                  ${PY_FILES}
                  INSTALL_DIRS
                  ${BIN_DIR}/mantid
                  ${WORKBENCH_BIN_DIR}/mantid)
# version.py that will overwrite the ones from the built target
mtd_install_files(FILES
                  ${CMAKE_CURRENT_BINARY_DIR}/${VERSION_PY}.install.py
                  INSTALL_DIRS
                  ${BIN_DIR}/mantid
                  ${WORKBENCH_BIN_DIR}/mantid
                  RENAME
                  ${VERSION_PY}.py)
