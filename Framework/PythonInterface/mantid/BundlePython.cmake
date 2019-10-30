###############################################################################
# BundlePython.cmake
#
# This file contains the code to bundle the Python distribution from third party
#
###############################################################################

if( MSVC )
  #####################################################################
  # Bundled Python for dev build. Assumes PYTHON_DIR has been set in
  # MSVCSetup.cmake
  #####################################################################
  file(TO_NATIVE_PATH ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR} BIN_NATIVE)
  add_custom_command( TARGET ${PYBUNDLE_POST_TARGET} POST_BUILD
                      COMMAND ${CMAKE_COMMAND}
                      ARGS -E copy_if_different ${PYTHON_DIR}/python27.dll ${BIN_NATIVE}/python27.dll
                      COMMENT "Copying Python27 dll to bin" )
 
  #####################################################################
  # Bundle for package
  #####################################################################
  install ( DIRECTORY ${PYTHON_DIR}/DLLs DESTINATION bin PATTERN ".svn" EXCLUDE PATTERN ".git" EXCLUDE
                                                                          PATTERN "*_d.pyd" EXCLUDE PATTERN "*_d.dll" EXCLUDE )
  install ( DIRECTORY ${PYTHON_DIR}/Lib DESTINATION bin PATTERN ".svn" EXCLUDE PATTERN ".git" EXCLUDE PATTERN "*_d.pyd" EXCLUDE )
  install ( DIRECTORY ${PYTHON_DIR}/Scripts DESTINATION bin PATTERN ".svn" EXCLUDE PATTERN ".git" EXCLUDE PATTERN "*_d.py" EXCLUDE )
  install ( DIRECTORY ${PYTHON_DIR}/tcl DESTINATION bin PATTERN ".svn" EXCLUDE PATTERN ".git" EXCLUDE )
  install ( FILES ${PYTHON_DIR}/python27.dll ${PYTHON_EXECUTABLE} ${PYTHONW_EXECUTABLE} DESTINATION bin )

# elseif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")

#   # Find Python framework
#   get_filename_component(PYTHON_VERSION_X_Y_DIR "${PYTHON_LIBRARY}" DIRECTORY)
#   get_filename_component(PYTHON_VERSION_X_Y_DIR "${PYTHON_VERSION_X_Y_DIR}" DIRECTORY)
#   set(PY_VER ${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR})
#   set(_site_packages /usr/local/lib/python${PY_VER}/site-packages)
#   if(NOT EXISTS "${_site_packages}")
#     message(FATAL_ERROR "Cannot find python site packages in ${_site_packages}. ")
#   endif()

#   # common libraries
#   set(BUNDLED_PACKAGES
#     backports
#     certifi
#     chardet
#     CifFile
#     dateutil
#     enum
#     h5py
#     idna
#     IPython
#     ipython_genutils
#     ipykernel
#     jupyter_core
#     jupyter_client
#     markupsafe
#     matplotlib
#     mpl_toolkits
#     numpy
#     pathlib2
#     pexpect
#     pkg_resources
#     prompt_toolkit
#     ptyprocess
#     pygments
#     qtconsole
#     qtpy
#     pygments
#     tornado
#     requests
#     scipy
#     sphinx
#     sphinx_bootstrap_theme
#     traitlets
#     urllib3
#     wcwidth
#     yaml
#     zmq
#   )
#   set(BUNDLED_PACKAGES_MANTIDPLOT
#     PyQt4
#   )
#   set(BUNDLED_PACKAGES_WORKBENCH
#     PyQt5
#   )
#   set(BUNDLED_MODULES
#     _posixsubprocess32.so
#     cycler.py
#     pyparsing.py
#     mistune.py
#     decorator.py
#     kiwisolver.so
#     pickleshare.py
#     scandir.py
#     simplegeneric.py
#     sip.so
#     six.py
#     subprocess32.py
#   )
  
#   function(install_packages destination src_dir)
#     foreach(_pkg ${ARGN})
#       if(IS_SYMLINK ${_pkg})
        
#       else
#         install(DIRECTORY "${src_dir}/${_pkg}"
#                 DESTINATION "${destination}")
#       endif()
#     endforeach()
#   endfunction()
  
#   foreach(_bundle ${BUNDLES})
#     # install python
#     install(DIRECTORY "${PYTHON_VERSION_X_Y_DIR}" 
#             DESTINATION ${_bundle}Frameworks/Python.framework/Versions
# 	    USE_SOURCE_PERMISSIONS
# 	    # we'll make our own site-packages
# 	    PATTERN "site-packages" EXCLUDE
#     )
#     # required third party libraries
#     set(_bundle_site_packages
#         ${_bundle}Frameworks/Python.framework/Versions/${PY_VER}/lib/python${PY_VER}/site-packages)
#     install_packages("${_bundle_site_packages}" "${_site_packages}" ${BUNDLED_PACKAGES})
#     if(${_bundle} MATCHES "^MantidPlot")
#       install_packages("${_bundle_site_packages}" "${_site_packages}" ${BUNDLED_PACKAGES_MANTIDPLOT})
#     elseif(${_bundle} MATCHES "^MantidWorkbench")
#       install_packages("${_bundle_site_packages}" "${_site_packages}" ${BUNDLED_PACKAGES_WORKBENCH})
#     endif()

#     foreach(_module ${BUNDLED_MODULES})
#       install(FILES "${_site_packages}/${_module}"
#               DESTINATION ${_bundle_site_packages})
#     endforeach()    
#   endforeach()
endif()
