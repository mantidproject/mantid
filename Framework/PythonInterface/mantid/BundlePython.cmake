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

endif()
