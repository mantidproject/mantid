###########################################################################
# Define scripts for the Linux packages
#
# It provides:
#  - launch_mantidplot.sh
#
###########################################################################

###########################################################################
# Set installation variables
###########################################################################
set ( BIN_DIR bin )
set ( ETC_DIR etc )
set ( LIB_DIR lib )
set ( PLUGINS_DIR plugins )
set ( PVPLUGINS_DIR pvplugins )
set ( PVPLUGINS_SUBDIR pvplugins ) # Need to tidy these things up!

if ( CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT )
  set ( CMAKE_INSTALL_PREFIX /opt/Mantid CACHE PATH "Install path" FORCE )
endif()

set ( CMAKE_INSTALL_RPATH ${CMAKE_INSTALL_PREFIX}/${LIB_DIR};${CMAKE_INSTALL_PREFIX}/${PLUGINS_DIR};${CMAKE_INSTALL_PREFIX}/${PVPLUGINS_DIR} )

###########################################################################
# LD_PRELOAD libraries
###########################################################################
set ( EXTRA_LDPRELOAD_LIBS "${TCMALLOC_LIBRARIES}" )

###########################################################################
# Environment scripts (profile.d)
###########################################################################
# default shell (bash-like)
file ( WRITE ${CMAKE_CURRENT_BINARY_DIR}/mantid.sh
  "#!/bin/sh\n"
  "MANTIDPATH=${CMAKE_INSTALL_PREFIX}/${BIN_DIR}\n"
  "PV_PLUGIN_PATH=${CMAKE_INSTALL_PREFIX}/${PVPLUGINS_DIR}/${PVPLUGINS_DIR}\n"
  "PATH=$PATH:$MANTIDPATH\n"
  "PYTHONPATH=$MANTIDPATH:$PYTHONPATH\n"
  "LD_PRELOAD=\n"
  "export MANTIDPATH PV_PLUGIN_PATH PATH PYTHONPATH LD_PRELOAD\n"
)

# c-shell
file ( WRITE ${CMAKE_CURRENT_BINARY_DIR}/mantid.csh
  "#!/bin/csh\n"
  "setenv MANTIDPATH \"${CMAKE_INSTALL_PREFIX}/${BIN_DIR}\"\n"
  "setenv PV_PLUGIN_PATH \"${CMAKE_INSTALL_PREFIX}/${PVPLUGINS_DIR}/${PVPLUGINS_DIR}\"\n"
  "setenv PATH \"\${PATH}:\${MANTIDPATH}\"\n"
  "setenv LD_PRELOAD \"\"\n"
  "if ($?PYTHONPATH) then\n"
  "  setenv PYTHONPATH \"\${MANTIDPATH}:\${PYTHONPATH}\"\n"
  "else\n"
  "  setenv PYTHONPATH \"\${MANTIDPATH}\"\n"
  "endif\n"
)

install ( PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/mantid.sh
  ${CMAKE_CURRENT_BINARY_DIR}/mantid.csh
  DESTINATION ${ETC_DIR}
)

############################################################################
# Setup file variables for pre/post installation
# These are very different depending on the distribution so are contained
# in the Packaging/*/scripts directory as CMake templates
############################################################################
if ( NOT MPI_BUILD )
  set ( ENVVARS_ON_INSTALL ON CACHE BOOL
        "Whether to include the scripts in /etc/profile.d to set the MANTIDPATH variable and add it to PATH. Turning this off allows installing locally without being root." )
endif()
# for shell maintainer scripts as ENVVARS_ON_INSTALL could have ON, OFF, True, False etc
if ( ENVVARS_ON_INSTALL )
  set ( ENVVARS_ON_INSTALL_INT 1 )
else ()
  set ( ENVVARS_ON_INSTALL_INT 0 )
endif()

# Common filenames to hold maintainer scripts
set ( PRE_INSTALL_FILE ${CMAKE_CURRENT_BINARY_DIR}/preinst )
set ( POST_INSTALL_FILE ${CMAKE_CURRENT_BINARY_DIR}/postinst )
set ( PRE_UNINSTALL_FILE ${CMAKE_CURRENT_BINARY_DIR}/prerm )
set ( PRE_UNINSTALL_FILE ${CMAKE_CURRENT_BINARY_DIR}/postrm )

if ( "${UNIX_DIST}" MATCHES "RedHatEnterprise" OR "${UNIX_DIST}" MATCHES "^Fedora" ) # RHEL/Fedora
  set ( PKG_INSTALL_PREFIX \$RPM_INSTALL_PREFIX0 ) # used in mainter scripts

  if ( NOT MPI_BUILD )
    configure_file ( ${CMAKE_MODULE_PATH}/Packaging/rpm/scripts/rpm_pre_install.sh.in
                     ${PRE_INSTALL_FILE} )
    configure_file ( ${CMAKE_MODULE_PATH}/Packaging/rpm/scripts/rpm_post_install.sh.in
                     ${POST_INSTALL_FILE} )
    configure_file ( ${CMAKE_MODULE_PATH}/Packaging/rpm/scripts/rpm_pre_uninstall.sh.in
                     ${PRE_UNINSTALL_FILE} )
    configure_file ( ${CMAKE_MODULE_PATH}/Packaging/rpm/scripts/rpm_post_install.sh.in
                     ${POST_UNINSTALL_FILE} )
    # CPack variables
    set ( CPACK_RPM_PRE_INSTALL_SCRIPT_FILE ${PRE_INSTALL_FILE} )
    set ( CPACK_RPM_POST_INSTALL_SCRIPT_FILE ${POST_INSTALL_FILE} )
    set ( CPACK_RPM_PRE_UNINSTALL_SCRIPT_FILE ${PRE_UNINSTALL_FILE} )
    set ( CPACK_RPM_POST_UNINSTALL_SCRIPT_FILE ${POST_UNINSTALL_FILE} )
  endif()

  if ( "${UNIX_CODENAME}" MATCHES "Santiago" ) # el6
    set ( MANTIDPLOT_LAUNCHER_PREFIX
          "LD_PRELOAD=\${EXTRA_LDPRELOAD_LIBS} scl enable mantidlibs" )
  else()
    set ( MANTIDPLOT_LAUNCHER_PREFIX
          "LD_PRELOAD=\${EXTRA_LDPRELOAD_LIBS} LD_LIBRARY_PATH=/usr/lib64/paraview:${LD_LIBRARY_PATH} eval" ) # LD_PATH hack for non-official ParaView
  endif()
elseif ( "${UNIX_DIST}" MATCHES "Ubuntu" )
  set ( MANTIDPLOT_LAUNCHER_PREFIX "LD_PRELOAD=${EXTRA_LDPRELOAD_LIBS} eval" )

  if ( NOT MPI_BUILD )
    configure_file ( ${CMAKE_MODULE_PATH}/Packaging/deb/scripts/deb_post_inst.in
                     ${POST_INSTALL_FILE} )
    # CPack variables
    set ( CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA
          "${POST_INSTALL_FILE}" )
  endif()
  # Launcher prefix
  set ( MANTIDPLOT_LAUNCHER_PREFIX "LD_PRELOAD=\${EXTRA_LDPRELOAD_LIBS} eval" )
endif()

############################################################################
# MantidPlot launcher script
############################################################################
# Local dev version
file ( WRITE ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/launch_mantidplot.sh "#!/bin/bash\n"
  "#\n"
  "# Launch Mantidplot using any necessary LD_PRELOAD or software collection behaviour\n"
  "#\n"
  "SCRIPTDIR=$(dirname $(readlink -f \"$0\"))\n"
  "${MANTIDPLOT_LAUNCHER_PREFIX} \"\$SCRIPTDIR/MantidPlot $*\" \n"
)
# Needs to be executable
execute_process ( COMMAND "chmod" "+x" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/launch_mantidplot.sh"
                  OUTPUT_QUIET ERROR_QUIET )
# Package version
file ( WRITE ${CMAKE_CURRENT_BINARY_DIR}/launch_mantidplot.sh.install "#!/bin/sh\n"
  "#\n"
  "# Launch Mantidplot using any necessary LD_PRELOAD or software collection behaviour\n"
  "#\n"
  "${MANTIDPLOT_LAUNCHER_PREFIX} \"${CMAKE_INSTALL_PREFIX}/${BIN_DIR}/MantidPlot_exe $*\" \n"
)

install ( FILES ${CMAKE_CURRENT_BINARY_DIR}/launch_mantidplot.sh.install
          DESTINATION ${BIN_DIR} RENAME launch_mantidplot.sh
          PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ
          GROUP_EXECUTE GROUP_READ
          WORLD_EXECUTE WORLD_READ
)
