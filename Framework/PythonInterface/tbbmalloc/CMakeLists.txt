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
  set(_installed_hook_files)
  foreach(_file ${_tbbmalloc_hook_files})
    get_filename_component(_name ${_file} NAME)
    list(APPEND _installed_hook_files ${_site_packages}/${_name})
  endforeach()
  if(_preload_tbbmalloc)
    add_custom_command(
      OUTPUT ${_installed_hook_files}
      COMMAND ${CMAKE_COMMAND} -E copy ${_tbbmalloc_hook_files} ${_site_packages}
      DEPENDS ${_tbbmalloc_hook_files}
      COMMENT "Installing mantid-tbbmalloc.pth into ${_site_packages}"
    )
    add_custom_target(TbbmallocDotPth ALL DEPENDS ${_installed_hook_files})
    add_dependencies(PythonInterface TbbmallocDotPth)
  else()
    # this build does not preload tbbmalloc, so remove the hook an earlier configuration copied into the environment
    file(REMOVE ${_installed_hook_files})
  endif()
endif()

if(_preload_tbbmalloc)
  install(
    FILES ${_tbbmalloc_hook_files}
    DESTINATION ${Python_SITELIB_RELPATH}
    COMPONENT Runtime
  )
endif()
