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
set ( SITE_PACKAGES ${LIB_DIR} )
set ( PLUGINS_DIR plugins )

set ( WORKBENCH_BIN_DIR ${BIN_DIR} )
set ( WORKBENCH_LIB_DIR ${LIB_DIR} )
set ( WORKBENCH_SITE_PACKAGES ${LIB_DIR} CACHE PATH "Location of site packages")
set ( WORKBENCH_PLUGINS_DIR ${PLUGINS_DIR} )

if ( CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT )
  set ( CMAKE_INSTALL_PREFIX /opt/mantid${CPACK_PACKAGE_SUFFIX} CACHE PATH "Install path" FORCE )
endif()

# Tell rpm to use the appropriate python executable
set(CPACK_RPM_SPEC_MORE_DEFINE "%define __python ${Python_EXECUTABLE}")

# Tell rpm that this package does not own /opt /usr/share/{applications,pixmaps}
# Required for Fedora >= 18 and RHEL >= 7
set ( CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST_ADDITION /opt /usr/share/applications /usr/share/pixmaps )

###########################################################################
# Environment scripts (profile.d)
###########################################################################
# default shell (bash-like)
file ( WRITE ${CMAKE_CURRENT_BINARY_DIR}/mantid.sh
  "#!/bin/sh\n"
  "PATH=$PATH:${CMAKE_INSTALL_PREFIX}/${BIN_DIR}\n"

  "export PATH\n"
)

# c-shell
file ( WRITE ${CMAKE_CURRENT_BINARY_DIR}/mantid.csh
  "#!/bin/csh\n"
  "setenv PATH \"\${PATH}:${CMAKE_INSTALL_PREFIX}/${BIN_DIR}\"\n"
)

install ( PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/mantid.sh
  ${CMAKE_CURRENT_BINARY_DIR}/mantid.csh
  DESTINATION ${ETC_DIR}
)

###########################################################################
# Find python site-packages dir and create mantid.pth
###########################################################################
execute_process(
  COMMAND "${Python_EXECUTABLE}" -c "from distutils import sysconfig as sc
print(sc.get_python_lib(plat_specific=True))"
  OUTPUT_VARIABLE PYTHON_SITE
  OUTPUT_STRIP_TRAILING_WHITESPACE)

file ( WRITE ${CMAKE_CURRENT_BINARY_DIR}/mantid.pth.install
  "${CMAKE_INSTALL_PREFIX}/${BIN_DIR}\n"
  "${CMAKE_INSTALL_PREFIX}/${LIB_DIR}\n"
)

install ( FILES ${CMAKE_CURRENT_BINARY_DIR}/mantid.pth.install
  DESTINATION ${ETC_DIR}
  RENAME mantid.pth
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
  # these are used if we need to scl enable at runtime
  set ( WRAPPER_PREFIX "" )
  set ( WRAPPER_POSTFIX "" )

  if ("${UNIX_DIST}" MATCHES "^Fedora")
    # The instrument view doesn't work with the wayland compositor
    # Override it to use X11 https://doc.qt.io/qt-5/embedded-linux.html#xcb
    set ( QT_QPA "QT_QPA_PLATFORM=xcb " )
  else()
    set ( QT_QPA "" )
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
if [ -n \"\${NXSESSIONID}\" ]; then  # running in nx
  command -v vglrun >/dev/null 2>&1 || { echo >&2 \"MantidPlot requires VirtualGL but it's not installed.  Aborting.\"; exit 1; }
  VGLRUN=\"vglrun\"
elif [ -n \"\${TLSESSIONDATA}\" ]; then  # running in thin-linc
  command -v vglrun >/dev/null 2>&1 || { echo >&2 \"MantidPlot requires VirtualGL but it's not installed.  Aborting.\"; exit 1; }
  if [ \$(command -v vgl-wrapper.sh) ]; then
    VGLRUN=\"vgl-wrapper.sh\"
  else
    VGLRUN=\"vglrun\"
  fi
fi" )

# The scripts need jemalloc to be resolved to the runtime library as the plain
# .so symlink is only present when a -dev/-devel package is presentz
if ( JEMALLOCLIB_FOUND )
  get_filename_component ( JEMALLOC_RUNTIME_LIB ${JEMALLOC_LIBRARIES} REALPATH )
  # We only want to use the major version number
  string( REGEX REPLACE "([0-9]+)\.[0-9]+\.[0-9]+$" "\\1" JEMALLOC_RUNTIME_LIB ${JEMALLOC_RUNTIME_LIB} )
endif ()

# definitions to preload jemalloc but not if we are using address sanitizer as this confuses things
if ( WITH_ASAN )
  set ( JEMALLOC_DEFINITIONS
"
LOCAL_PRELOAD=\${LD_PRELOAD}
"
)
else ()
  # Do not indent the string below as it messes up the formatting in the final script
  set ( JEMALLOC_DEFINITIONS
"# Define parameters for jemalloc
LOCAL_PRELOAD=${JEMALLOC_RUNTIME_LIB}
if [ -n \"\${LD_PRELOAD}\" ]; then
    LOCAL_PRELOAD=\${LOCAL_PRELOAD}:\${LD_PRELOAD}
fi
" )
endif()

# chunk of code for launching gdb
set ( GDB_DEFINITIONS
"# run with gdb THIS OPTION MUST BE SUPPLIED FIRST
if [ -n \"\$1\" ] && [ \"\$1\" = \"--debug\" ]; then
    shift
    GDB=\"gdb --args\"
fi" )

set ( ERROR_CMD "-m mantidqt.dialogs.errorreports.main --exitcode=\$?" )

##### Local dev version
set ( PYTHON_ARGS "-Wdefault::DeprecationWarning -Werror:::mantid -Werror:::mantidqt" )

set ( LOCAL_PYPATH "\${INSTALLDIR}/bin" )

# used by mantidworkbench
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

##### Package version
unset ( PYTHON_ARGS )

# used by mantidplot and mantidworkbench
set ( LOCAL_PYPATH "\${INSTALLDIR}/bin:\${INSTALLDIR}/lib:\${INSTALLDIR}/plugins" )

if (ENABLE_WORKBENCH)
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
