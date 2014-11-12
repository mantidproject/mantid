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
  "LD_PRELOAD=${EXTRA_LDPRELOAD_LIBS}:$LD_PRELOAD\n"
  "export MANTIDPATH PV_PLUGIN_PATH PATH PYTHONPATH LD_PRELOAD\n"
)

# c-shell
file ( WRITE ${CMAKE_CURRENT_BINARY_DIR}/mantid.csh
  "#!/bin/csh\n"
  "setenv MANTIDPATH \"${CMAKE_INSTALL_PREFIX}/${BIN_DIR}\"\n"
  "setenv PV_PLUGIN_PATH \"${CMAKE_INSTALL_PREFIX}/${PVPLUGINS_DIR}/${PVPLUGINS_DIR}\"\n"
  "setenv PATH \"\${PATH}:\${MANTIDPATH}\"\n"
  "setenv LD_PRELOAD \"${EXTRA_LDPRELOAD_LIBS}:\${LD_PRELOAD}\"\n"
  "if ($?PYTHONPATH) then\n"
  "  setenv PYTHONPATH \"\${MANTIDPATH}:\${PYTHONPATH}\"\n"
  "else\n"
  "  setenv PYTHONPATH \"\${MANTIDPATH}\"\n"
  "endif\n"
)

############################################################################
# Pre/Post install/uninstall scripts
############################################################################

if ( "${UNIX_DIST}" MATCHES "RedHatEnterprise" OR "${UNIX_DIST}" MATCHES "^Fedora" ) # RHEL/Fedora
  file ( WRITE ${CMAKE_CURRENT_BINARY_DIR}/rpm_post_install.sh
    "#!/bin/sh\n"
    "if [ ! -e $RPM_INSTALL_PREFIX0/${BIN_DIR}/mantidplot ]; then\n"
    "  ln -s $RPM_INSTALL_PREFIX0/${BIN_DIR}/MantidPlot $RPM_INSTALL_PREFIX0/${BIN_DIR}/mantidplot\n"
    "fi\n"
    "if [ -f $RPM_INSTALL_PREFIX0/${BIN_DIR}/MantidPlot ]; then\n"
    "  mv $RPM_INSTALL_PREFIX0/${BIN_DIR}/MantidPlot $RPM_INSTALL_PREFIX0/${BIN_DIR}/MantidPlot_exe\n"
    "  ln -s $RPM_INSTALL_PREFIX0/${BIN_DIR}/launch_mantidplot.sh $RPM_INSTALL_PREFIX0/${BIN_DIR}/MantidPlot\n"
    "fi\n"
  )
  # Link profile scripts on install
  if ( ENVVARS_ON_INSTALL )
    file (APPEND ${CMAKE_CURRENT_BINARY_DIR}/rpm_post_install.sh
      "\n"
      "ln -s $RPM_INSTALL_PREFIX0/${ETC_DIR}/mantid.sh /etc/profile.d/mantid.sh\n"
      "ln -s $RPM_INSTALL_PREFIX0/${ETC_DIR}/mantid.csh /etc/profile.d/mantid.csh\n"
    )
  endif()
  # Uninstall
  file ( WRITE ${CMAKE_CURRENT_BINARY_DIR}/rpm_pre_uninstall.sh "#!/bin/sh\n"
    "if [ ! -f $RPM_INSTALL_PREFIX0/${PVPLUGINS_DIR}/${PVPLUGINS_DIR}/libMantidParaViewSplatterPlotSMPlugin.so ];then\n"
    "  rm -f $RPM_INSTALL_PREFIX0/${BIN_DIR}/mantidplot\n"
    "fi\n"
    "if [ -h /etc/profile.d/mantid.sh ]; then\n"
    "  rm /etc/profile.d/mantid.sh\n"
    "fi\n"
    "if [ -h /etc/profile.d/mantid.csh ]; then\n"
    "  rm /etc/profile.d/mantid.csh\n"
    "fi\n"
    "if [ -f $RPM_INSTALL_PREFIX0/${BIN_DIR}MantidPlot_exe  ]; then\n"
    "  rm $RPM_INSTALL_PREFIX0/${BIN_DIR}/MantidPlot_exe\n"
    "fi\n"
  )

  file ( WRITE ${CMAKE_CURRENT_BINARY_DIR}/rpm_remove_all_links.sh "#!/bin/sh\n"
    "if [ ! -f $RPM_INSTALL_PREFIX0/${PVPLUGINS_DIR}/${PVPLUGINS_DIR}/libMantidParaViewSplatterPlotSMPlugin.so ];then\n"
    "  rm -f $RPM_INSTALL_PREFIX0/${BIN_DIR}/mantidplot\n"
    "fi\n"
    "if [ -h /etc/profile.d/mantid.sh ]; then\n"
    "  rm /etc/profile.d/mantid.sh\n"
    "fi\n"
    "if [ -h /etc/profile.d/mantid.csh ]; then\n"
    "  rm /etc/profile.d/mantid.csh\n"
    "fi\n"
  )
  file ( WRITE ${CMAKE_CURRENT_BINARY_DIR}/rpm_remove_links.sh "#!/bin/sh\n"
    "if [ ! -f $RPM_INSTALL_PREFIX0/${PVPLUGINS_DIR}/${PVPLUGINS_DIR}/libMantidParaViewSplatterPlotSMPlugin.so ];then\n"
    "  rm -f $RPM_INSTALL_PREFIX0/${BIN_DIR}/mantidplot\n"
    "fi\n"
    )
  file ( WRITE ${CMAKE_CURRENT_BINARY_DIR}/rpm_remove_empty_install.sh "#!/bin/sh\n"
    "# If the install prefix contains mantid then prune empty directories.\n"
    "# Begin extra cautious here just in case some has set the something like Prefix=/usr\n"
    "if echo \"$RPM_INSTALL_PREFIX0\" | grep -qi mantid; then\n"
    "  find $RPM_INSTALL_PREFIX0 -mindepth 1 -type d -empty -delete\n"
    "  rmdir --ignore-fail-on-non-empty -p $RPM_INSTALL_PREFIX0\n"
    "else\n"
    "    echo Install prefix does not contain the word mantid. Empty directories NOT removed.\n"
    "    exit 1\n"
    "fi\n"
  )
  if ( "${UNIX_CODENAME}" MATCHES "Santiago" ) # el6
    file ( WRITE ${CMAKE_CURRENT_BINARY_DIR}/launch_mantidplot.sh "#!/bin/sh\n"
           "scl enable mantidlibs \"${CMAKE_INSTALL_PREFIX}/${BIN_DIR}/MantidPlot_exe $*\" \n"
         )
  else()
    file ( WRITE ${CMAKE_CURRENT_BINARY_DIR}/launch_mantidplot.sh "#!/bin/sh\n"
           "LD_LIBRARY_PATH=/usr/lib64/paraview:${LD_LIBRARY_PATH} ${CMAKE_INSTALL_PREFIX}/${BIN_DIR}/MantidPlot_exe $* \n"
         )
  endif()

  install ( FILES  ${CMAKE_CURRENT_BINARY_DIR}/launch_mantidplot.sh
            DESTINATION ${BIN_DIR}
            PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ
                        GROUP_EXECUTE GROUP_READ
                        WORLD_EXECUTE WORLD_READ
  )
  install ( PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/mantid.sh 
                     ${CMAKE_CURRENT_BINARY_DIR}/mantid.csh
            DESTINATION ${ETC_DIR}
  )
endif()

# unset all install/uninstall scripts
unset ( CPACK_RPM_PRE_INSTALL_SCRIPT_FILE )
unset ( CPACK_RPM_POST_INSTALL_SCRIPT_FILE )
unset ( CPACK_RPM_PRE_UNINSTALL_SCRIPT_FILE )
unset ( CPACK_RPM_POST_UNINSTALL_SCRIPT_FILE )

# set install/uninstall scripts as desired
if ( NOT MPI_BUILD )
  set ( ENVVARS_ON_INSTALL ON CACHE BOOL "Whether to include the scripts in /etc/profile.d to set the MANTIDPATH variable and add it to PATH. Turning this off allows installing locally without being root." )
  if ( ENVVARS_ON_INSTALL )
    set ( CPACK_RPM_PRE_INSTALL_SCRIPT_FILE ${CMAKE_CURRENT_BINARY_DIR}/rpm_remove_all_links.sh )
    set ( CPACK_RPM_POST_INSTALL_SCRIPT_FILE ${CMAKE_CURRENT_BINARY_DIR}/rpm_post_install.sh )
    set ( CPACK_RPM_PRE_UNINSTALL_SCRIPT_FILE ${CMAKE_CURRENT_BINARY_DIR}/rpm_pre_uninstall.sh )
    set ( CPACK_RPM_POST_UNINSTALL_SCRIPT_FILE ${CMAKE_CURRENT_BINARY_DIR}/rpm_remove_empty_install.sh )
  else ( ENVVARS_ON_INSTALL )
    set ( CPACK_RPM_POST_INSTALL_SCRIPT_FILE ${CMAKE_CURRENT_BINARY_DIR}/rpm_post_install.sh )
    set ( CPACK_RPM_PRE_UNINSTALL_SCRIPT_FILE ${CMAKE_CURRENT_BINARY_DIR}/rpm_remove_links.sh )
    set ( CPACK_RPM_POST_UNINSTALL_SCRIPT_FILE ${CMAKE_CURRENT_BINARY_DIR}/rpm_remove_empty_install.sh )
  endif ()
endif ( NOT MPI_BUILD )

