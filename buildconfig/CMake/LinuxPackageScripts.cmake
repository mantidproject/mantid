# ######################################################################################################################
# Installation variables that control the destination subdirectories for install() commands
# ######################################################################################################################
set(BIN_DIR bin)
set(ETC_DIR etc)
set(LIB_DIR lib)
set(SITE_PACKAGES ${LIB_DIR})
set(PLUGINS_DIR plugins)

set(WORKBENCH_BIN_DIR ${BIN_DIR})
set(WORKBENCH_LIB_DIR ${LIB_DIR})
set(WORKBENCH_SITE_PACKAGES
    ${LIB_DIR}
    CACHE PATH "Location of site packages"
)
set(WORKBENCH_PLUGINS_DIR ${PLUGINS_DIR})

# ######################################################################################################################
# Launcher scripts. We provide a wrapper script to launch workbench to:
#
# * enable a custom memory allocator (jemalloc)
# * enable VirtualGL configuration if required for remote access
# * adds debug flags to start gdb for developers
# ######################################################################################################################
set(CONDA_PREAMBLE_TEXT
    "# Verify that conda is setup
if [ -z \"\${CONDA_PREFIX}\" ]; then
    echo \"CONDA_PREFIX is not defined\"
    echo \"The mantid conda environment does not appear to be enabled\"
    exit 1
fi
"
)
set(SYS_PREAMBLE_TEXT
    "# Find out where we are
THISFILE=\$(readlink -f \"\$0\")
INSTALLDIR=\$(dirname \$THISFILE)   # directory of executable
INSTALLDIR=\$(dirname \$INSTALLDIR) # root install directory
"
)

# common definition of work for virtualgl - lots of escaping things from cmake
set(VIRTUAL_GL_WRAPPER
    "# whether or not to use vglrun
if [ -n \"\${NXSESSIONID}\" ]; then  # running in nx
  command -v vglrun >/dev/null 2>&1 || { echo >&2 \"mantidworkbench requires VirtualGL but it's not installed.  Aborting.\"; exit 1; }
  VGLRUN=\"vglrun\"
elif [ -n \"\${TLSESSIONDATA}\" ]; then  # running in thin-linc
  command -v vglrun >/dev/null 2>&1 || { echo >&2 \"mantidworkbench requires VirtualGL but it's not installed.  Aborting.\"; exit 1; }
  if [ \$(command -v vgl-wrapper.sh) ]; then
    VGLRUN=\"vgl-wrapper.sh\"
  else
    VGLRUN=\"vglrun\"
  fi
fi"
)

# The scripts need jemalloc to be resolved to the runtime library as the plain .so symlink is only present when a
# -dev/-devel package is presentz
if(JEMALLOCLIB_FOUND)
  get_filename_component(JEMALLOC_RUNTIME_LIB ${JEMALLOC_LIBRARIES} REALPATH)
  # We only want to use the major version number
  string(REGEX REPLACE "([0-9]+)\.[0-9]+\.[0-9]+$" "\\1" JEMALLOC_RUNTIME_LIB ${JEMALLOC_RUNTIME_LIB})
endif()

# definitions to preload jemalloc but not if we are using address sanitizer as this confuses things
if(WITH_ASAN)
  set(JEMALLOC_DEFINITIONS
      "
LOCAL_PRELOAD=\${LD_PRELOAD}
"
  )
else()
  # Do not indent the string below as it messes up the formatting in the final script
  set(JEMALLOC_DEFINITIONS
      "# Define parameters for jemalloc
LOCAL_PRELOAD=${JEMALLOC_RUNTIME_LIB}
if [ -n \"\${LD_PRELOAD}\" ]; then
    LOCAL_PRELOAD=\${LOCAL_PRELOAD}:\${LD_PRELOAD}
fi
"
  )
endif()

# chunk of code for launching gdb
set(GDB_DEFINITIONS
    "# run with gdb THIS OPTION MUST BE SUPPLIED FIRST
if [ -n \"\$1\" ] && [ \"\$1\" = \"--debug\" ]; then
    shift
    GDB=\"gdb --args\"
    SINGLEPROCESS=\"--single-process\"
fi"
)

set(ERROR_CMD "-m mantidqt.dialogs.errorreports.main --exitcode=\$?")

# Local dev version
set(PYTHON_ARGS " -Wdefault::DeprecationWarning -Werror:::mantid -Werror:::mantidqt")

set(PYTHON_EXEC_LOCAL "\${CONDA_PREFIX}/bin/python")
set(PREAMBLE "${CONDA_PREAMBLE_TEXT}")
set(LOCAL_PYPATH "${CMAKE_CURRENT_BINARY_DIR}/bin/")

# used by mantidworkbench
if(ENABLE_WORKBENCH)
  set(MANTIDWORKBENCH_EXEC "-m workbench") # what the actual thing is called
  configure_file(
    ${CMAKE_MODULE_PATH}/Packaging/launch_mantidworkbench.sh.in
    ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/launch_mantidworkbench.sh @ONLY
  )
  # Needs to be executable
  execute_process(
    COMMAND "chmod" "+x" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/launch_mantidworkbench.sh" OUTPUT_QUIET ERROR_QUIET
  )
endif()
configure_file(
  ${CMAKE_MODULE_PATH}/Packaging/AddPythonPath.py.in ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/AddPythonPath.py @ONLY
)
# Needs to be executable
execute_process(COMMAND "chmod" "+x" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/AddPythonPath.py" OUTPUT_QUIET ERROR_QUIET)

# Package version
unset(PYTHON_ARGS)

if(ENABLE_WORKBENCH)
  foreach(install_type conda;standalone)
    # used by mantidworkbench
    if(${install_type} STREQUAL "conda")
      set(LOCAL_PYPATH "\${CONDA_PREFIX}/bin:\${CONDA_PREFIX}/lib:\${CONDA_PREFIX}/plugins")
      set(PYTHON_EXEC_LOCAL "\${CONDA_PREFIX}/bin/python")
      set(PREAMBLE "${CONDA_PREAMBLE_TEXT}")
      set(MANTIDWORKBENCH_EXEC "-m workbench") # what the actual thing is called
      set(DEST_FILENAME_SUFFIX "")
    elseif(${install_type} STREQUAL "standalone")
      set(LOCAL_PYPATH "\${INSTALLDIR}/bin:\${INSTALLDIR}/lib:\${INSTALLDIR}/plugins")
      set(PYTHON_EXEC_LOCAL "\${INSTALLDIR}/bin/python")
      set(PREAMBLE "${SYS_PREAMBLE_TEXT}")
      set(MANTIDWORKBENCH_EXEC "-m workbench")
      set(DEST_FILENAME_SUFFIX ".standalone")
    else()
      message(FATAL_ERROR "Unknown installation type '${install_type}' for workbench startup scripts")
    endif()
    # workbench launcher for jemalloc
    configure_file(
      ${CMAKE_MODULE_PATH}/Packaging/launch_mantidworkbench.sh.in
      ${CMAKE_CURRENT_BINARY_DIR}/launch_mantidworkbench.sh.install${DEST_FILENAME_SUFFIX} @ONLY
    )
    install(
      PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/launch_mantidworkbench.sh.install${DEST_FILENAME_SUFFIX}
      DESTINATION ${BIN_DIR}
      RENAME mantidworkbench${DEST_FILENAME_SUFFIX}
    )
  endforeach()
endif()
