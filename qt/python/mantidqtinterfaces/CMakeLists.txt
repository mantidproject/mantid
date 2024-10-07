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

# Subdirectories from which ui files need processing to py files FilterEvents doesn't need any special work
add_subdirectory(mantidqtinterfaces/ui)

# Chain all required interface custom targets into CompilePyUI
add_custom_target(CompilePyUI DEPENDS CompileUIUI)

# Put them into the 'CompileUI' folder or group in VS and the like, for convenience
set_property(TARGET CompilePyUI PROPERTY FOLDER "CompilePyUI")
set_property(TARGET CompileUIUI PROPERTY FOLDER "CompilePyUI")

# Setup dependency chain
if(BUILD_MANTIDFRAMEWORK)
  add_dependencies(mantidqtinterfaces PythonInterface)
endif()

if(BUILD_MANTIDQT)
  add_dependencies(mantidqtinterfaces mantidqt)
endif()

add_subdirectory(test)
add_subdirectory(mantidqtinterfaces/Engineering/gui)
add_subdirectory(mantidqtinterfaces/sans_isis/gui_logic/test)
add_subdirectory(mantidqtinterfaces/drill)
