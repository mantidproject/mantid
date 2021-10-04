# Defines functions to help deal with python packages

# ~~~
# Function to create links to python packages from the source tree
# to the binary tree. It expects a setup.py to be found in
# ${CMAKE_CURRENT_SOURCE_DIR}/setup.py
# Optional keyword arguments:
#   - EXECUTABLE: If this option provided then it is assumed the package contains a
#                 startup script and this is installed in the package bin
#                 directory
#   - EGGLINKNAME: Pass in a new name for the egg link, e.g. EGGLINKNAME mylink,
#                  creates a new egg link called mylink
#   - INSTALL_LIB_DIRS: A list of install directories.
#   - INSTALL_BIN_DIR: Destination for an executable to be installed
#   - EXCLUDE_ON_INSTALL: Specifies a regex of files to exclude from the install
#   -                     command
#   - GENERATE_SITECUSTOMIZE: If provided then generate a sitecustomize
#                             file in the egg link directory
# ~~~
function(add_python_package pkg_name)
  set(_setup_py ${CMAKE_CURRENT_SOURCE_DIR}/setup.py)
  set(_setup_py_build_root ${CMAKE_CURRENT_SOURCE_DIR}/build)
  set(_egg_link_dir ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR})

  cmake_parse_arguments(
    _parsed_arg "EXECUTABLE;GENERATE_SITECUSTOMIZE"
    "EGGLINKNAME;EXCLUDE_FROM_INSTALL;INSTALL_BIN_DIR" "INSTALL_LIB_DIRS"
    ${ARGN}
  )

  if(_parsed_arg_EGGLINKNAME)
    set(_egg_link ${_egg_link_dir}/${_parsed_arg_EGGLINKNAME}.egg-link)
  else()
    # if no egg-link name is specified, then use the target name
    set(_egg_link ${_egg_link_dir}/${pkg_name}.egg-link)
  endif()

  if(_parsed_arg_EXECUTABLE)
    if(WIN32)
      # add .exe in the executable name for Windows, otherwise it can't find it
      # during the install step
      set(_executable_name ${pkg_name}.exe)
      set(_startup_script_full_name ${pkg_name}-script.pyw)
      set(_startup_script ${_egg_link_dir}/${_startup_script_full_name})
    else()
      set(_startup_script_full_name)
      set(_startup_script)
      set(_executable_name ${pkg_name})
    endif()
    set(_startup_exe ${_egg_link_dir}/${_executable_name})
  endif()

  # create the developer setup which just creates a pth file rather than copying
  # things over
  set(_outputs ${_egg_link} ${_startup_script} ${_startup_exe})
  set(_version_str
      ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}${VERSION_TWEAK_PY}
  )
  add_custom_command(
    OUTPUT ${_outputs}
    COMMAND
      ${CMAKE_COMMAND} -E env PYTHONPATH=${_egg_link_dir}
      MANTID_VERSION_STR=${_version_str} ${Python_EXECUTABLE} ${_setup_py}
      develop --install-dir ${_egg_link_dir} --script-dir ${_egg_link_dir}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS ${_setup_py}
  )

  # Generate a sitecustomize.py file in the egg link directory as setuptools no
  # longer generates site.py for v>=49.0.0
  if(_parsed_arg_GENERATE_SITECUSTOMIZE AND Python_SETUPTOOLS_VERSION
                                            VERSION_GREATER_EQUAL 49.0.0
  )
    add_custom_command(
      OUTPUT ${_egg_link_dir}/sitecustomize.py
      COMMAND ${CMAKE_COMMAND} -DSITECUSTOMIZE_DIR=${_egg_link_dir} -P
              ${CMAKE_MODULE_PATH}/WriteSiteCustomize.cmake
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
      DEPENDS ${_setup_py} ${CMAKE_MODULE_PATH}/WriteSiteCustomize.cmake
    )
    list(APPEND _outputs ${_egg_link_dir}/sitecustomize.py)
  endif()

  add_custom_target(${pkg_name} ALL DEPENDS ${_outputs})

  # setuptools by default wants to build into a directory called 'build'
  # relative the to the working directory. We have overridden commands in
  # setup.py.in to force the build directory to take place out of source. The
  # install directory is specified here and then --install-scripts=bin
  # --install-lib=lib removes any of the platform/distribution specific install
  # directories so we can have a flat structure
  if (CONDA_BUILD)
  install(
    CODE "execute_process(COMMAND ${CMAKE_COMMAND} -E env MANTID_VERSION_STR=${_version_str} \
    ${Python_EXECUTABLE} -m pip install ${CMAKE_CURRENT_SOURCE_DIR} --no-deps --ignore-installed --no-cache-dir -vvv)"
  )
  else()
  install(
    CODE "execute_process(COMMAND ${CMAKE_COMMAND} -E env MANTID_VERSION_STR=${_version_str} \
    ${Python_EXECUTABLE} ${_setup_py} install -O1 --single-version-externally-managed \
    --root=${_setup_py_build_root}/install --install-scripts=bin --install-lib=lib \
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})"
  )
  endif()

  if (NOT CONDA_BUILD)
  # Registers the "installed" components with CMake so it will carry them over
  if(_parsed_arg_EXCLUDE_FROM_INSTALL)
    foreach(_dest ${_parsed_arg_INSTALL_LIB_DIRS})
      install(
        DIRECTORY ${_setup_py_build_root}/install/lib/
        DESTINATION ${_dest}
        PATTERN "test" EXCLUDE
        REGEX "${_parsed_arg_EXCLUDE_FROM_INSTALL}" EXCLUDE
      )
    endforeach()
  else()
    foreach(_dest ${_parsed_arg_INSTALL_LIB_DIRS})
      install(
        DIRECTORY ${_setup_py_build_root}/install/lib/
        DESTINATION ${_dest}
        PATTERN "test" EXCLUDE
      )
    endforeach()
  endif()
  # install the generated executable
  if(_parsed_arg_EXECUTABLE AND _parsed_arg_INSTALL_BIN_DIR)
    install(PROGRAMS ${_setup_py_build_root}/install/bin/${pkg_name}
            DESTINATION ${_parsed_arg_INSTALL_BIN_DIR}
    )
  endif()
  endif()
endfunction()
