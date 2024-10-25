# External projects
include(PyStoG)

# .pth files
set(_pth_dirs
    .
    Calibration
    DiamondAttenuationCorrection
    Diffraction
    Engineering
    GSAS-II
    Inelastic
    Interface
    Reflectometry
    SANS
    SCD_Reduction
    TemporaryREF_MScripts
)

set(_pth_list_dev)
set(_pth_list_install)
set(_scripts_rel_path "../scripts")

foreach(_dir ${_pth_dirs})
  list(APPEND _pth_list_dev "${CMAKE_SOURCE_DIR}/scripts/${_dir}")
  list(APPEND _pth_list_install "${_scripts_rel_path}/${_dir}")
endforeach()
list(APPEND _pth_list_dev ${CMAKE_CURRENT_BINARY_DIR})

# development copy
set(_scripts_pth_src "${CMAKE_CURRENT_BINARY_DIR}/mantid-scripts.pth.src")
set(_scripts_pth_dest "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}/mantid-scripts.pth")
string(REPLACE ";" "\n" _pth_list_dev "${_pth_list_dev}")
file(WRITE ${_scripts_pth_src} "${_pth_list_dev}\n")
add_custom_command(
  OUTPUT ${_scripts_pth_dest}
  COMMAND ${CMAKE_COMMAND} -E copy_if_different ${_scripts_pth_src} ${_scripts_pth_dest}
  DEPENDS ${_scripts_pth_src}
  COMMENT "Generating scripts .pth file"
)
add_custom_target(ScriptsDotPth ALL DEPENDS ${_scripts_pth_dest})
add_dependencies(PythonInterface ScriptsDotPth)

# install copy
set(_scripts_pth_install "${CMAKE_CURRENT_BINARY_DIR}/mantid-scripts.pth.install")
string(REPLACE ";" "\n" _pth_list_install "${_pth_list_install}")
file(WRITE ${_scripts_pth_install} "${_pth_list_install}\n")

mtd_install_files(
  FILES
  ${_scripts_pth_install}
  INSTALL_DIRS
  ${BIN_DIR}
  ${WORKBENCH_BIN_DIR}
  RENAME
  mantid-scripts.pth
)

# Testing
add_subdirectory(test)
add_subdirectory(Diffraction/isis_powder)
add_subdirectory(Diffraction/single_crystal)
add_subdirectory(Engineering)

# Ensure we don't get stale pyc files around
clean_orphaned_pyc_files(${CMAKE_CURRENT_SOURCE_DIR})
