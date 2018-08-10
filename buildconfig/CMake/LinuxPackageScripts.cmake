###########################################################################
# Define scripts for the Linux packages
#
# It provides:
#  - launch_mantidplot.sh
#  - launch_mantidworkbench.sh
#  - mantid.sh <- for stable releases
#  - mantid.csh <- for stable releases
#
###########################################################################

###########################################################################
# Set installation variables
###########################################################################
set ( BIN_DIR bin )
set ( ETC_DIR etc )
set ( LIB_DIR lib )
# This is the root of the plugins directory
set ( PLUGINS_DIR plugins )
# Separate directory of plugins to be discovered by the ParaView framework
# These cannot be mixed with our other plugins. Further sub-directories
# based on the Qt version will also be created by the installation targets
set ( PVPLUGINS_DIR "plugins/paraview/qt4/" )

if ( CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT )
  set ( CMAKE_INSTALL_PREFIX /opt/mantid${CPACK_PACKAGE_SUFFIX} CACHE PATH "Install path" FORCE )
endif()

# Tell rpm that this package does not own /opt /usr/share/{applications,pixmaps}
# Required for Fedora >= 18 and RHEL >= 7
set ( CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST_ADDITION /opt /usr/share/applications /usr/share/pixmaps )

###########################################################################
# Environment scripts (profile.d)
###########################################################################
# default shell (bash-like)
file ( WRITE ${CMAKE_CURRENT_BINARY_DIR}/mantid.sh
  "#!/bin/sh\n"
  "MANTIDPATH=${CMAKE_INSTALL_PREFIX}/${BIN_DIR}\n"
  "PV_PLUGIN_PATH=${CMAKE_INSTALL_PREFIX}/${PVPLUGINS_DIR}\n"
  "PATH=$PATH:$MANTIDPATH\n"

  "export MANTIDPATH PV_PLUGIN_PATH PATH\n"
)

# c-shell
file ( WRITE ${CMAKE_CURRENT_BINARY_DIR}/mantid.csh
  "#!/bin/csh\n"
  "setenv MANTIDPATH \"${CMAKE_INSTALL_PREFIX}/${BIN_DIR}\"\n"
  "setenv PV_PLUGIN_PATH \"${CMAKE_INSTALL_PREFIX}/${PVPLUGINS_DIR}\"\n"
  "setenv PATH \"\${PATH}:\${MANTIDPATH}\"\n"
)

install ( PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/mantid.sh
  ${CMAKE_CURRENT_BINARY_DIR}/mantid.csh
  ${CMAKE_CURRENT_BINARY_DIR}/mantid.pth
  DESTINATION ${ETC_DIR}
)

###########################################################################
# Find python site-packages dir and create mantid.pth
###########################################################################
execute_process(
  COMMAND "${PYTHON_EXECUTABLE}" -c "from distutils import sysconfig as sc
print(sc.get_python_lib(plat_specific=True))"
  OUTPUT_VARIABLE PYTHON_SITE
  OUTPUT_STRIP_TRAILING_WHITESPACE)

file ( WRITE ${CMAKE_CURRENT_BINARY_DIR}/mantid.pth
  "${CMAKE_INSTALL_PREFIX}/${BIN_DIR}\n"
)

############################################################################
# Setup file variables for pre/post installation
# These are very different depending on the distribution so are contained
# in the Packaging/*/scripts directory as CMake templates
############################################################################
# We only manipulate the environment for  nprefixed non-MPI package builds
# for shell maintainer scripts as ENVVARS_ON_INSTALL could have ON, OFF, True, False etc
if ( MPI_BUILD OR CPACK_PACKAGE_SUFFIX )
  set ( ENVVARS_ON_INSTALL_INT 0 )
else ()
  set ( ENVVARS_ON_INSTALL_INT 1 )
endif()

# Common filenames to hold maintainer scripts
set ( PRE_INSTALL_FILE ${CMAKE_CURRENT_BINARY_DIR}/preinst )
set ( POST_INSTALL_FILE ${CMAKE_CURRENT_BINARY_DIR}/postinst )
set ( PRE_UNINSTALL_FILE ${CMAKE_CURRENT_BINARY_DIR}/prerm )
set ( POST_UNINSTALL_FILE ${CMAKE_CURRENT_BINARY_DIR}/postrm )

if ( "${UNIX_DIST}" MATCHES "RedHatEnterprise" OR "${UNIX_DIST}" MATCHES "^Fedora" ) # RHEL/Fedora
  if ( "${UNIX_CODENAME}" MATCHES "Santiago" )
    set ( WRAPPER_PREFIX "scl enable mantidlibs34 \"" )
    set ( WRAPPER_POSTFIX "\"" )
  else()
    set ( WRAPPER_PREFIX "" )
    set ( WRAPPER_POSTFIX "" )
  endif()

  if ( NOT MPI_BUILD )
    configure_file ( ${CMAKE_MODULE_PATH}/Packaging/rpm/scripts/rpm_pre_install.sh.in
                     ${PRE_INSTALL_FILE} @ONLY )
    configure_file ( ${CMAKE_MODULE_PATH}/Packaging/rpm/scripts/rpm_post_install.sh.in
                     ${POST_INSTALL_FILE} @ONLY )
    configure_file ( ${CMAKE_MODULE_PATH}/Packaging/rpm/scripts/rpm_pre_uninstall.sh.in
                     ${PRE_UNINSTALL_FILE} @ONLY )
    configure_file ( ${CMAKE_MODULE_PATH}/Packaging/rpm/scripts/rpm_post_uninstall.sh.in
                     ${POST_UNINSTALL_FILE} @ONLY )
    # CPack variables
    set ( CPACK_RPM_PRE_INSTALL_SCRIPT_FILE ${PRE_INSTALL_FILE} )
    set ( CPACK_RPM_POST_INSTALL_SCRIPT_FILE ${POST_INSTALL_FILE} )
    set ( CPACK_RPM_PRE_UNINSTALL_SCRIPT_FILE ${PRE_UNINSTALL_FILE} )
    set ( CPACK_RPM_POST_UNINSTALL_SCRIPT_FILE ${POST_UNINSTALL_FILE} )
  endif()
elseif ( "${UNIX_DIST}" MATCHES "Ubuntu" )
  set ( WRAPPER_PREFIX "" )
  set ( WRAPPER_POSTFIX "" )

  if ( NOT MPI_BUILD )
    configure_file ( ${CMAKE_MODULE_PATH}/Packaging/deb/scripts/deb_pre_inst.in
                     ${PRE_INSTALL_FILE} @ONLY )
    configure_file ( ${CMAKE_MODULE_PATH}/Packaging/deb/scripts/deb_post_inst.in
                     ${POST_INSTALL_FILE} @ONLY )
    configure_file ( ${CMAKE_MODULE_PATH}/Packaging/deb/scripts/deb_pre_rm.in
                     ${PRE_UNINSTALL_FILE} @ONLY )
    # No postrm script as dpkg removes empty directories if everything else is tidied away.

    # CPack variables
    set ( CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA
          "${PRE_INSTALL_FILE};${POST_INSTALL_FILE};${PRE_UNINSTALL_FILE}" )
  endif()
endif()

############################################################################
# Launcher scripts
############################################################################
# common definition of work for virtualgl - lots of escaping things from cmake
set ( VIRTUAL_GL_WRAPPER
"# whether or not to use vglrun
if [ -n \"\${NXSESSIONID}\" ]; then
  command -v vglrun >/dev/null 2>&1 || { echo >&2 \"MantidPlot requires VirtualGL but it's not installed.  Aborting.\"; exit 1; }
  VGLRUN=\"vglrun\"
elif [ -n \"\${TLSESSIONDATA}\" ]; then
  command -v vglrun >/dev/null 2>&1 || { echo >&2 \"MantidPlot requires VirtualGL but it's not installed.  Aborting.\"; exit 1; }
  VGLRUN=\"vglrun\"
fi" )

# The scripts need tcmalloc to be resolved to the runtime library as the plain
# .so symlink is only present when a -dev/-devel package is present
if ( TCMALLOC_FOUND )
  get_filename_component ( TCMALLOC_RUNTIME_LIB ${TCMALLOC_LIBRARIES} REALPATH )
  # We only want to use the major version number
  string( REGEX REPLACE "([0-9]+)\.[0-9]+\.[0-9]+$" "\\1" TCMALLOC_RUNTIME_LIB ${TCMALLOC_RUNTIME_LIB} )
endif ()

# definitions to preload tcmalloc
set ( TCMALLOC_DEFINITIONS
"# Define parameters for tcmalloc
LOCAL_PRELOAD=${TCMALLOC_RUNTIME_LIB}
if [ -n \"\${LD_PRELOAD}\" ]; then
    LOCAL_PRELOAD=\${LOCAL_PRELOAD}:\${LD_PRELOAD}
fi
if [ -z \"\${TCMALLOC_RELEASE_RATE}\" ]; then
    TCM_RELEASE=10000
else
    TCM_RELEASE=\${TCMALLOC_RELEASE_RATE}
fi

# Define when to report large memory allocation
if [ -z \"\${TCMALLOC_LARGE_ALLOC_REPORT_THRESHOLD}\" ]; then
    # total available memory
    TCM_REPORT=\$(grep MemTotal /proc/meminfo --color=never | awk '{print \$2}')
    # half of available memory
    TCM_REPORT=`expr 512 \\* \$TCM_REPORT`
    # minimum is 1GB
    if [ \${TCM_REPORT} -le 1073741824 ]; then
        TCM_REPORT=1073741824
    fi
else
    TCM_REPORT=\${TCMALLOC_LARGE_ALLOC_REPORT_THRESHOLD}
fi" )

# chunk of code for fixing MANTIDPATH
set ( MTD_PATH_DEFINITION "MANTIDPATH=\${INSTALLDIR}/bin" )

# chunk of code for launching gdb
set ( GDB_DEFINITIONS
"# run with gdb THIS OPTION MUST BE SUPPLIED FIRST
if [ -n \"\$1\" ] && [ \"\$1\" = \"--debug\" ]; then
    shift
    GDB=\"gdb --args\"
fi" )

set ( ERROR_CMD "ErrorReporter/error_dialog_app.py --exitcode=\$? --directory=\$INSTALLDIR/bin" )

# Local dev version
if ( MAKE_VATES )
  set ( PARAVIEW_PYTHON_PATHS ":${ParaView_DIR}/lib:${ParaView_DIR}/lib/site-packages" )
else ()
  set ( PARAVIEW_PYTHON_PATHS "" )
endif ()

set ( LOCAL_PYPATH "\${INSTALLDIR}/bin" )
set ( SCRIPTSDIR ${CMAKE_HOME_DIRECTORY}/scripts)

# used by mantidplot and mantidworkbench
if (ENABLE_MANTIDPLOT)
  set ( MANTIDPLOT_EXEC MantidPlot )
  configure_file ( ${CMAKE_MODULE_PATH}/Packaging/launch_mantidplot.sh.in
                   ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/launch_mantidplot.sh @ONLY )
  # Needs to be executable
  execute_process ( COMMAND "chmod" "+x" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/launch_mantidplot.sh"
                    OUTPUT_QUIET ERROR_QUIET )
endif ()
if (ENABLE_WORKBENCH)
  set ( MANTIDWORKBENCH_EXEC workbench ) # what the actual thing is called
  configure_file ( ${CMAKE_MODULE_PATH}/Packaging/launch_mantidworkbench.sh.in
                   ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/launch_mantidworkbench.sh @ONLY )
  # Needs to be executable
  execute_process ( COMMAND "chmod" "+x" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/launch_mantidworkbench.sh"
                    OUTPUT_QUIET ERROR_QUIET )
endif()
configure_file ( ${CMAKE_MODULE_PATH}/Packaging/mantidpython.in
                 ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/mantidpython @ONLY )
# Needs to be executable
execute_process ( COMMAND "chmod" "+x" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/mantidpython"
                  OUTPUT_QUIET ERROR_QUIET )
configure_file ( ${CMAKE_MODULE_PATH}/Packaging/AddPythonPath.py.in
                 ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/AddPythonPath.py @ONLY )
# Needs to be executable
execute_process ( COMMAND "chmod" "+x" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/AddPythonPath.py"
                  OUTPUT_QUIET ERROR_QUIET )

# Package version
if ( MAKE_VATES )
  set ( EXTRA_LDPATH "\${INSTALLDIR}/lib/paraview-${ParaView_VERSION_MAJOR}.${ParaView_VERSION_MINOR}" )
  set ( PV_PYTHON_PATH "\${INSTALLDIR}/lib/paraview-${ParaView_VERSION_MAJOR}.${ParaView_VERSION_MINOR}" )
  set ( PARAVIEW_PYTHON_PATHS ":${PV_PYTHON_PATH}:${PV_PYTHON_PATH}/site-packages:${PV_PYTHON_PATH}/site-packages/vtk" )
else ()
  set ( PARAVIEW_PYTHON_PATHS "" )
endif ()

# used by mantidplot and mantidworkbench
set ( LOCAL_PYPATH "\${INSTALLDIR}/lib:\${INSTALLDIR}/plugins" )
set ( SCRIPTSDIR "\${INSTALLDIR}/scripts")

if (ENABLE_MANTIDPLOT)
  set ( MANTIDPLOT_EXEC MantidPlot_exe )
  configure_file ( ${CMAKE_MODULE_PATH}/Packaging/launch_mantidplot.sh.in
                   ${CMAKE_CURRENT_BINARY_DIR}/launch_mantidplot.sh.install @ONLY )
  install ( PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/launch_mantidplot.sh.install
            DESTINATION ${BIN_DIR} RENAME mantidplot )
endif ()
if (PACKAGE_WORKBENCH) # will eventually switch to ENABLE_WORKBENCH
  set ( MANTIDWORKBENCH_EXEC workbench ) # what the actual thing is called
  configure_file ( ${CMAKE_MODULE_PATH}/Packaging/launch_mantidworkbench.sh.in
                   ${CMAKE_CURRENT_BINARY_DIR}/launch_mantidworkbench.sh.install @ONLY )
  install ( PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/launch_mantidworkbench.sh.install
            DESTINATION ${BIN_DIR} RENAME mantidworkbench )
endif()
configure_file ( ${CMAKE_MODULE_PATH}/Packaging/mantidpython.in
                 ${CMAKE_CURRENT_BINARY_DIR}/mantidpython.install @ONLY )
install ( PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/mantidpython.install
          DESTINATION ${BIN_DIR} RENAME mantidpython )
