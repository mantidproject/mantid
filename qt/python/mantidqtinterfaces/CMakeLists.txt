# This file manages building/installation of the mantid scientific interfaces
include(PythonPackageTargetFunctions)

# mantidqt
if(APPLE)
  set(_install_lib_dirs)
  if(ENABLE_WORKBENCH)
    list(APPEND _install_lib_dirs ${WORKBENCH_SITE_PACKAGES})
  endif()
else()
  set(_install_lib_dirs "${SITE_PACKAGES}")
endif()

add_python_package(mantidqtinterfaces INSTALL_LIB_DIRS ${_install_lib_dirs})

# Setup dependency chain
add_dependencies(mantidqtinterfaces PythonInterface)

add_subdirectory(mantidqtinterfaces/test)
