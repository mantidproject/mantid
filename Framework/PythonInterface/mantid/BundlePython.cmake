###############################################################################
# BundlePython.cmake
#
# This file contains the code to bundle the Python distribution from third party
#
###############################################################################

if( MSVC )
  #####################################################################
  # Bundle for package
  #####################################################################
  install ( DIRECTORY ${PYTHON_DIR}/DLLs DESTINATION bin PATTERN ".svn" EXCLUDE PATTERN ".git" EXCLUDE
                                                                          PATTERN "*_d.pyd" EXCLUDE PATTERN "*_d.dll" EXCLUDE )
  install ( DIRECTORY ${PYTHON_DIR}/Lib DESTINATION bin PATTERN ".svn" EXCLUDE PATTERN ".git" EXCLUDE PATTERN "*_d.pyd" EXCLUDE )
  install ( DIRECTORY ${PYTHON_DIR}/Scripts DESTINATION bin PATTERN ".svn" EXCLUDE PATTERN ".git" EXCLUDE PATTERN "*_d.py" EXCLUDE )
  install ( DIRECTORY ${PYTHON_DIR}/tcl DESTINATION bin PATTERN ".svn" EXCLUDE PATTERN ".git" EXCLUDE )
  install ( FILES ${PYTHON_DIR}/python${PYTHON_VERSION_MAJOR}${PYTHON_VERSION_MINOR}.dll
            ${PYTHON_EXECUTABLE} ${PYTHONW_EXECUTABLE}
            DESTINATION bin )
  # Python >= 3 has an minimal API DLL too
  if ( EXISTS ${PYTHON_DIR}/python${PYTHON_VERSION_MAJOR}.dll )
    install ( FILES ${PYTHON_DIR}/python${PYTHON_VERSION_MAJOR}.dll
              DESTINATION bin )
  endif()
endif()
