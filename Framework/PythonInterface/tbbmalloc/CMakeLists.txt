# mantid-tbbmalloc.pth restarts python, ipython and Jupyter kernels with tbbmalloc preloaded, as launch_mantidworkbench
# does. A .pth file only runs from a site directory, so developer builds copy it into the site-packages of the
# environment, next to the editable installs of the python packages, and packages install it into their site-packages.
set(_tbbmalloc_hook_files ${CMAKE_CURRENT_SOURCE_DIR}/mantid-tbbmalloc.pth
                          ${CMAKE_CURRENT_SOURCE_DIR}/_mantid_tbbmalloc_preload.py
)
string(TOLOWER "${USE_SANITIZER}" _sanitizers_lower)
if(USE_TBBMALLOC
   AND TBBMALLOC_RUNTIME_LIB
   AND NOT _sanitizers_lower MATCHES "address"
)
  set(_preload_tbbmalloc TRUE)
else()
  set(_preload_tbbmalloc FALSE)
endif()

if(NOT CONDA_BUILD)
  execute_process(
    COMMAND ${Python_EXECUTABLE} -c "import sysconfig; print(sysconfig.get_path('purelib'))"
    OUTPUT_VARIABLE _site_packages
    OUTPUT_STRIP_TRAILING_WHITESPACE COMMAND_ERROR_IS_FATAL ANY
  )
  set(_installed_pth ${_site_packages}/mantid-tbbmalloc.pth)
  set(_installed_module ${_site_packages}/_mantid_tbbmalloc_preload.py)
  # The copied .pth names this build directory, so that only the build that copied the hook removes it. Several build
  # directories can share one environment, and the environment can also hold the hook from a mantid conda package.
  set(_owner_line "# installed by the Mantid build in ${CMAKE_BINARY_DIR}")
  if(_preload_tbbmalloc)
    file(READ ${CMAKE_CURRENT_SOURCE_DIR}/mantid-tbbmalloc.pth _pth_contents)
    file(CONFIGURE OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/mantid-tbbmalloc.pth CONTENT "${_owner_line}\n${_pth_contents}"
         @ONLY
    )
    set(_hook_sources ${CMAKE_CURRENT_BINARY_DIR}/mantid-tbbmalloc.pth
                      ${CMAKE_CURRENT_SOURCE_DIR}/_mantid_tbbmalloc_preload.py
    )
    add_custom_command(
      OUTPUT ${_installed_pth} ${_installed_module}
      COMMAND ${CMAKE_COMMAND} -E copy ${_hook_sources} ${_site_packages}
      DEPENDS ${_hook_sources}
      COMMENT "Installing mantid-tbbmalloc.pth into ${_site_packages}"
    )
    add_custom_target(TbbmallocDotPth ALL DEPENDS ${_installed_pth} ${_installed_module})
    add_dependencies(PythonInterface TbbmallocDotPth)
  elseif(EXISTS ${_installed_pth})
    # this build does not preload tbbmalloc, so remove the hook if this build directory copied it into the environment
    file(STRINGS ${_installed_pth} _installed_lines)
    if("${_owner_line}" IN_LIST _installed_lines)
      file(REMOVE ${_installed_pth} ${_installed_module})
    else()
      message(STATUS "Leaving ${_installed_pth}, which this build directory did not install")
    endif()
  endif()
endif()

if(_preload_tbbmalloc)
  install(
    FILES ${_tbbmalloc_hook_files}
    DESTINATION ${Python_SITELIB_RELPATH}
    COMPONENT Runtime
  )
endif()
