# Subdirectories from which ui files need processing to py files FilterEvents
# doesn't need any special work
add_subdirectory(Interface/ui)
add_subdirectory(ErrorReporter)

# Chain all required interface custom targets into CompilePyUI
add_custom_target(CompilePyUI
		  DEPENDS CompileUIUI
		          CompileUIErrorReporter)

# Put them into the 'CompileUI' folder or group in VS and the like, for
# convenience
set_property(TARGET CompilePyUI PROPERTY FOLDER "CompilePyUI")
set_property(TARGET CompileUIUI PROPERTY FOLDER "CompilePyUI")
set_property(TARGET CompileUIErrorReporter PROPERTY FOLDER "CompilePyUI")

# External GUIs
add_subdirectory(ExternalInterfaces)

# External projects
include(PyStoG)

# .pth files
set(_pth_dirs .
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
    Vates)

set(_pth_list_dev)
set(_pth_list_install)
if(APPLE)
  set(_scripts_rel_path "../../scripts")
else()
  set(_scripts_rel_path "../scripts")
endif()

foreach(_dir ${_pth_dirs})
  list(APPEND _pth_list_dev "${CMAKE_SOURCE_DIR}/scripts/${_dir}")
  list(APPEND _pth_list_install "${_scripts_rel_path}/${_dir}")
endforeach()
list(APPEND _pth_list_dev ${CMAKE_CURRENT_BINARY_DIR})
list(APPEND _pth_list_dev ${MSLICE_DEV})
list(APPEND _pth_list_install "${_scripts_rel_path}/ExternalInterfaces")

# development copy
set(_scripts_pth_src "${CMAKE_CURRENT_BINARY_DIR}/mantid-scripts.pth.src")
set(_scripts_pth_dest
    "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}/mantid-scripts.pth")
string(REPLACE ";"
               "\n"
               _pth_list_dev
               "${_pth_list_dev}")
file(WRITE ${_scripts_pth_src} "${_pth_list_dev}\n")
add_custom_command(OUTPUT ${_scripts_pth_dest}
                   COMMAND ${CMAKE_COMMAND} -E copy_if_different
                           ${_scripts_pth_src} ${_scripts_pth_dest}
                   DEPENDS ${_scripts_pth_src}
                   COMMENT "Generating scripts .pth file")
add_custom_target(ScriptsDotPth ALL DEPENDS ${_scripts_pth_dest})
add_dependencies(PythonInterface ScriptsDotPth)

# install copy
set(_scripts_pth_install
    "${CMAKE_CURRENT_BINARY_DIR}/mantid-scripts.pth.install")
string(REPLACE ";"
               "\n"
               _pth_list_install
               "${_pth_list_install}")
file(WRITE ${_scripts_pth_install} "${_pth_list_install}\n")

install(FILES ${_scripts_pth_install}
        DESTINATION ${BIN_DIR}
        RENAME mantid-scripts.pth)
install(FILES ${_scripts_pth_install}
        DESTINATION ${WORKBENCH_BIN_DIR}
        RENAME mantid-scripts.pth)

# Testing
add_subdirectory(test)

# Ensure we don't get stale pyc files around
clean_orphaned_pyc_files(${CMAKE_CURRENT_SOURCE_DIR})
