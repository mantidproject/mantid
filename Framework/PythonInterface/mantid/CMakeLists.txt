# PythonInterface sub modules

# Generate the buildconfig module
set(BUILDCONFIG_PY buildconfig.py)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/${BUILDCONFIG_PY}.in
               ${CMAKE_CURRENT_SOURCE_DIR}/${BUILDCONFIG_PY})

# There is no library target for the top level package just the python files
add_custom_target(PythonModule
                  DEPENDS ${PYTHON_PKGROOT_INSTALL_FILES} ${PYTHON_EXE_WRAPPER}
                  SOURCES ${PY_FILES})
set_property(TARGET PythonModule PROPERTY FOLDER "MantidFramework/Python")

# Sub modules

add_subdirectory(kernel)
# Ensure the module files are in place
add_dependencies(PythonKernelModule PythonModule)
add_subdirectory(geometry)
add_subdirectory(api)
add_subdirectory(dataobjects)
add_subdirectory(_plugins)

# Defines the target the will cause the Python bundle to be copied
set(PYBUNDLE_POST_TARGET PythonModule)
include(${CMAKE_CURRENT_SOURCE_DIR}/BundlePython.cmake)
