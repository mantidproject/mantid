###########################################################################
# Use the system-installed version of Python.
###########################################################################
find_package ( PythonLibs REQUIRED )
include_directories ( ${PYTHON_INCLUDE_PATH} )
# If found, need to add debug library into libraries variable
if ( PYTHON_DEBUG_LIBRARIES )
  set ( PYTHON_LIBRARIES optimized ${PYTHON_LIBRARIES} debug ${PYTHON_DEBUG_LIBRARIES} )
endif ()

###########################################################################
# tcmalloc stuff. Only used on linux for now.
###########################################################################

# Look for tcmalloc. Make it optional, for now at least, but on by default
set ( USE_TCMALLOC ON CACHE BOOL "Flag for replacing regular malloc with tcmalloc" )
# If not wanted, just carry on without it
if ( USE_TCMALLOC )
  find_package ( Tcmalloc )
  if ( TCMALLOC_FOUND )
    set ( TCMALLOC_LIBRARY ${TCMALLOC_LIBRARIES} )
    # Make a C++ define to use as flags in, e.g. MemoryManager.cpp
    add_definitions ( -DUSE_TCMALLOC )
  else ( TCMALLOC_FOUND )
    # If not found, print a message telling the user to either get it or disable its use in the cache
    message ( SEND_ERROR "TCMalloc not found: either install the google-perftools suite on your system or set the USE_TCMALLOC CMake cache variable to OFF" ) 
  endif ( TCMALLOC_FOUND )
else ( USE_TCMALLOC )
  message ( STATUS "Not using TCMalloc" )
endif ( USE_TCMALLOC )

###########################################################################
# Set installation variables
###########################################################################

set ( BIN_DIR bin )
set ( ETC_DIR etc )
set ( LIB_DIR lib )
set ( PLUGINS_DIR plugins )
set ( PVPLUGINS_DIR pvplugins )

if ( CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT )
  set ( CMAKE_INSTALL_PREFIX /opt/Mantid CACHE PATH "Install path" FORCE )
endif()

set ( CMAKE_INSTALL_RPATH ${CMAKE_INSTALL_PREFIX}/${LIB_DIR};${CMAKE_INSTALL_PREFIX}/${PLUGINS_DIR};${CMAKE_INSTALL_PREFIX}/${PVPLUGINS_DIR}  )

file ( WRITE ${CMAKE_CURRENT_BINARY_DIR}/mantid.sh  "#!/bin/sh\n"
                                                    "MANTIDPATH=${CMAKE_INSTALL_PREFIX}/${BIN_DIR}\n"
                                                    "PV_PLUGIN_PATH=${CMAKE_INSTALL_PREFIX}/${PVPLUGINS_DIR}\n"
#                                                    "scriptpath=\"$(cd \"\${0%/*}\" 2> /dev/null;echo \"$PWD\"/\"\${0##*/}\")\"\n"
#                                                    "scriptpath=`readlink -f $scriptpath`\n"
#                                                    "scriptpath=`dirname $scriptpath`\n"
#                                                    "echo \"script $scriptpath\"\n"
#                                                    "MANTIDPATH=$scriptpath/${BIN_DIR}\n"
#                                                    "echo $MANTIDPATH\n"
#                                                    "PV_PLUGIN_PATH=$scriptpath/${PVPLUGINS_DIR}\n"
                                                    "PATH=$PATH:$MANTIDPATH\n"
                                                    "export MANTIDPATH PV_PLUGIN_PATH PATH\n"
)

file ( WRITE ${CMAKE_CURRENT_BINARY_DIR}/mantid.csh  "#!/bin/csh\n"
                                                    "setenv MANTIDPATH \"${CMAKE_INSTALL_PREFIX}/${BIN_DIR}\"\n"
                                                    "setenv PV_PLUGIN_PATH \"${CMAKE_INSTALL_PREFIX}/${PVPLUGINS_DIR}\"\n"
                                                    "setenv PATH \"\${PATH}:\${MANTIDPATH}\"\n"
)

file ( WRITE ${CMAKE_CURRENT_BINARY_DIR}/rpm_make_all_links.sh "#!/bin/sh\n"
                                                         "if [ ! -e $RPM_INSTALL_PREFIX0/${BIN_DIR}/mantidplot ]; then\n"
                                                         "  ln -s $RPM_INSTALL_PREFIX0/${BIN_DIR}/MantidPlot $RPM_INSTALL_PREFIX0/${BIN_DIR}/mantidplot\n"
                                                         "fi\n"
                                                         "ln -s $RPM_INSTALL_PREFIX0/${ETC_DIR}/mantid.sh /etc/profile.d/mantid.sh\n"
                                                         "ln -s $RPM_INSTALL_PREFIX0/${ETC_DIR}/mantid.csh /etc/profile.d/mantid.csh\n"
)

file ( WRITE ${CMAKE_CURRENT_BINARY_DIR}/rpm_make_links.sh "#!/bin/sh\n"
                                                         "if [ ! -e $RPM_INSTALL_PREFIX0/${BIN_DIR}/mantidplot ]; then\n"
                                                         "  ln -s $RPM_INSTALL_PREFIX0/${BIN_DIR}/MantidPlot $RPM_INSTALL_PREFIX0/${BIN_DIR}/mantidplot\n"
                                                         "fi"
)

file ( WRITE ${CMAKE_CURRENT_BINARY_DIR}/rpm_remove_all_links.sh "#!/bin/sh\n"
                                                             "if [ ! -f $RPM_INSTALL_PREFIX0/${PVPLUGINS_DIR}/libMantidParaViewSplatterPlotSMPlugin.so ];then\n"
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
                                                             "if [ ! -f $RPM_INSTALL_PREFIX0/${PVPLUGINS_DIR}/libMantidParaViewSplatterPlotSMPlugin.so ];then\n"
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

# Note: On older versions of CMake, this line may mean that to do a "make package" without being root
# you will need to set the cache variable CPACK_SET_DESTDIR to ON.
install ( PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/mantid.sh ${CMAKE_CURRENT_BINARY_DIR}/mantid.csh
          DESTINATION ${CMAKE_INSTALL_PREFIX}/${ETC_DIR}
)

# unset all install/uninstall scripts
unset ( CPACK_RPM_PRE_INSTALL_SCRIPT_FILE )
unset ( CPACK_RPM_POST_INSTALL_SCRIPT_FILE )
unset ( CPACK_RPM_PRE_UNINSTALL_SCRIPT_FILE )
unset ( CPACK_RPM_POST_UNINSTALL_SCRIPT_FILE )

# set install/uninstall scripts as desired
set ( ENVVARS_ON_INSTALL ON CACHE BOOL "Whether to include the scripts in /etc/profile.d to set the MANTIDPATH variable and add it to PATH. Turning this off allows installing locally without being root." )
if ( ENVVARS_ON_INSTALL )
  set ( CPACK_RPM_PRE_INSTALL_SCRIPT_FILE ${CMAKE_CURRENT_BINARY_DIR}/rpm_remove_all_links.sh )
  set ( CPACK_RPM_POST_INSTALL_SCRIPT_FILE ${CMAKE_CURRENT_BINARY_DIR}/rpm_make_all_links.sh )
  set ( CPACK_RPM_PRE_UNINSTALL_SCRIPT_FILE ${CMAKE_CURRENT_BINARY_DIR}/rpm_remove_all_links.sh )
  set ( CPACK_RPM_POST_UNINSTALL_SCRIPT_FILE ${CMAKE_CURRENT_BINARY_DIR}/rpm_remove_empty_install.sh )
else ( ENVVARS_ON_INSTALL )
  set ( CPACK_RPM_POST_INSTALL_SCRIPT_FILE ${CMAKE_CURRENT_BINARY_DIR}/rpm_make_links.sh )
  set ( CPACK_RPM_PRE_UNINSTALL_SCRIPT_FILE ${CMAKE_CURRENT_BINARY_DIR}/rpm_remove_links.sh )
  set ( CPACK_RPM_POST_UNINSTALL_SCRIPT_FILE ${CMAKE_CURRENT_BINARY_DIR}/rpm_remove_empty_install.sh )
endif ()
