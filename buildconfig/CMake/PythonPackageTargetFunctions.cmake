# Defines functions to help deal with python packages

# ~~~
# Function to create links to python packages in the source tree
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
# ~~~
function(
  add_python_package
  pkg_name
)
  # Create a setup.py file if necessary
  set(_setup_py ${CMAKE_CURRENT_SOURCE_DIR}/setup.py)
  set(_setup_py_build_root ${CMAKE_CURRENT_BINARY_DIR})

  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/setup.py.in")
    set(
      SETUPTOOLS_BUILD_COMMANDS_DEF
      "def patch_setuptools_command(cmd_cls_name):
    import importlib
    cmd_module = importlib.import_module('setuptools.command.' + cmd_cls_name)
    setuptools_command_cls = getattr(cmd_module, cmd_cls_name)

    class CustomCommand(setuptools_command_cls):
        user_options = setuptools_command_cls.user_options[:]
        boolean_options = setuptools_command_cls.boolean_options[:]
        def finalize_options(self):
            build_cmd = self.get_finalized_command('build')
            self.build_lib = '${_setup_py_build_root}/build'
            setuptools_command_cls.finalize_options(self)

    return CustomCommand

CustomBuildPy = patch_setuptools_command('build_py')
CustomInstall = patch_setuptools_command('install')
CustomInstallLib = patch_setuptools_command('install_lib')
"
    )
    set(
      SETUPTOOLS_BUILD_COMMANDS_USE
      "cmdclass={'build_py': CustomBuildPy, 'install': CustomInstall, 'install-lib': CustomInstallLib }"
    )
    configure_file(
      ${CMAKE_CURRENT_SOURCE_DIR}/setup.py.in
      ${_setup_py}
      @ONLY
    )
  endif()

  set(_egg_link_dir ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR})

  # Create variables for additional arguments
  cmake_parse_arguments(
    _parsed_arg
    "EXECUTABLE"
    "EGGLINKNAME;EXCLUDE_FROM_INSTALL;INSTALL_BIN_DIR"
    "INSTALL_LIB_DIRS"
    ${ARGN}
  )

  # If a custom egg-link name was specified use that for the link
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
  set(
    _outputs
    ${_egg_link}
    ${_startup_script}
    ${_startup_exe}
  )
  add_custom_command(
    OUTPUT ${_outputs}
    COMMAND
      ${CMAKE_COMMAND}
      -E
      env
      PYTHONPATH=${_egg_link_dir}
      ${PYTHON_EXECUTABLE}
      ${_setup_py}
      develop
      --install-dir
      ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}
      --script-dir
      ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS ${_setup_py}
  )
  add_custom_target(
    ${pkg_name}
    ALL
    DEPENDS ${_outputs}
  )

  # setuptools by default wants to build into a directory called 'build'
  # relative the to the working directory. We have overridden commands in
  # setup.py.in to force the build directory to take place out of source. The
  # install directory is specified here and then --install-scripts=bin
  # --install-lib=lib removes any of the platform/distribution specific install
  # directories so we can have a flat structure
  install(
    CODE
      "execute_process(COMMAND ${PYTHON_EXECUTABLE} ${_setup_py} install -O1 --single-version-externally-managed --root=${_setup_py_build_root}/install --install-scripts=bin --install-lib=lib WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})"
  )

  # Registers the "installed" components with CMake so it will carry them over
  if(_parsed_arg_EXCLUDE_FROM_INSTALL)
    foreach(
      _dest
      ${_parsed_arg_INSTALL_LIB_DIRS}
    )
      install(
        DIRECTORY ${_setup_py_build_root}/install/lib/
        DESTINATION ${_dest}
        PATTERN
          "test"
          EXCLUDE
        REGEX
          "${_parsed_arg_EXCLUDE_FROM_INSTALL}"
          EXCLUDE
      )
    endforeach()
  else()
    foreach(
      _dest
      ${_parsed_arg_INSTALL_LIB_DIRS}
    )
      install(
        DIRECTORY ${_setup_py_build_root}/install/lib/
        DESTINATION ${_dest}
        PATTERN
          "test"
          EXCLUDE
      )
    endforeach()
  endif()

  if(APPLE AND "${pkg_name}" STREQUAL "mantidqt")
    # Horrible hack to get mantidqt into the MantidPlot.app bundle too. Remove
    # this when MantidPlot is removed!! Registers the "installed" components
    # with CMake so it will carry them over
    install(
      DIRECTORY ${_setup_py_build_root}/install/lib/
      DESTINATION ${BIN_DIR}
      PATTERN
        "test"
        EXCLUDE
    )
  endif()

  # install the generated executable - only tested with "workbench"
  if(_parsed_arg_EXECUTABLE)
    # On UNIX systems install the workbench executable directly. The Windows
    # case is handled with a custom startup script installed in WindowsNSIS
    if(NOT WIN32)
      install(
        PROGRAMS ${_setup_py_build_root}/install/bin/${pkg_name}
        DESTINATION ${_parsed_arg_INSTALL_BIN_DIR}
        RENAME workbench-script
      )
    endif()
  endif()
endfunction()
