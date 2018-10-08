# Defines functions to help deal with python packages

# Function to create links to python packages in the source tree
# If the EXECUTABLE option is provided then it additional build rules are
# defined to ensure startup scripts are regenerated appropriately
function ( add_python_package pkg_name )
  # Create a setup.py file if necessary
  set ( _setup_py ${CMAKE_CURRENT_SOURCE_DIR}/setup.py )
  set ( _setup_py_build_root ${CMAKE_CURRENT_BINARY_DIR} )
  if ( EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/setup.py.in" )
    set ( SETUPTOOLS_BUILD_COMMANDS_DEF
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
" )
    set ( SETUPTOOLS_BUILD_COMMANDS_USE "cmdclass={'build_py': CustomBuildPy, 'install': CustomInstall, 'install-lib': CustomInstallLib }" )
    configure_file ( ${CMAKE_CURRENT_SOURCE_DIR}/setup.py.in ${_setup_py} @ONLY )
  endif ()

  set ( _egg_link_dir ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR} )
  set ( _egg_link ${_egg_link_dir}/${pkg_name}.egg-link )

  if ( ARGC GREATER 1 AND "${ARGN}" STREQUAL "EXECUTABLE" )
      if ( WIN32 )
        set ( _startup_script ${_egg_link_dir}/${pkg_name}-script.pyw )
        set ( _startup_exe ${_egg_link_dir}/${pkg_name}.exe )
      else ()
        set ( _startup_script )
        set ( _startup_exe ${_egg_link_dir}/${pkg_name} )
      endif ()
  endif ()

  # create the developer setup which just creates a pth file rather than copying things over
  set ( _outputs ${_egg_link} ${_startup_script} ${_startup_exe} )
  add_custom_command ( OUTPUT ${_outputs}
    COMMAND ${CMAKE_COMMAND} -E env PYTHONPATH=${_egg_link_dir}
      ${PYTHON_EXECUTABLE} ${_setup_py} develop
      --install-dir ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}
      --script-dir ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS ${_setup_py}
  )
  add_custom_target ( ${pkg_name} ALL
    DEPENDS ${_outputs}
  )

  if ( ${PACKAGE_WORKBENCH} )
    # setuptools by default wants to build into a directory called 'build' relative the to the working directory. We have overridden
    # commands in setup.py.in to force the build directory to take place out of source. The install directory is specified here and then
    # --install-scripts=bin --install-lib=lib removes any of the plaform/distribution specific install directories so we can have a flat
    # structure
    install(CODE "execute_process(COMMAND ${PYTHON_EXECUTABLE} ${_setup_py} install -O1 --single-version-externally-managed --root=${_setup_py_build_root}/install --install-scripts=bin --install-lib=lib WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})")
    # register the "installed" components with cmake so it will carry them over
    install(DIRECTORY ${_setup_py_build_root}/install/lib
            DESTINATION .
            PATTERN "test" EXCLUDE )

    # install the generated executable - only tested with "workbench"
    if ( ARGC GREATER 1 AND "${ARGN}" STREQUAL "EXECUTABLE" )
      install(PROGRAMS ${_setup_py_build_root}/install/bin/${pkg_name}
        DESTINATION bin)
    endif()
  endif()
endfunction ()
