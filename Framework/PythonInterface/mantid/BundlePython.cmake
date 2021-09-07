###############################################################################
# BundlePython.cmake
#
# This file contains the code to bundle the Python distribution from third party
#
###############################################################################

if( MSVC AND NOT CONDA_BUILD)
  #####################################################################
  # Bundle for package
  #####################################################################
  install ( DIRECTORY ${MSVC_PYTHON_EXECUTABLE_DIR}/DLLs DESTINATION bin PATTERN ".svn" EXCLUDE PATTERN ".git" EXCLUDE
                                                                          PATTERN "*_d.pyd" EXCLUDE PATTERN "*_d.dll" EXCLUDE )
  install ( DIRECTORY ${MSVC_PYTHON_EXECUTABLE_DIR}/Lib DESTINATION bin PATTERN ".svn" EXCLUDE PATTERN ".git" EXCLUDE PATTERN "*_d.pyd" EXCLUDE )
  install ( DIRECTORY ${MSVC_PYTHON_EXECUTABLE_DIR}/Scripts DESTINATION bin PATTERN ".svn" EXCLUDE PATTERN ".git" EXCLUDE PATTERN "*_d.py" EXCLUDE )
  install ( DIRECTORY ${MSVC_PYTHON_EXECUTABLE_DIR}/tcl DESTINATION bin PATTERN ".svn" EXCLUDE PATTERN ".git" EXCLUDE )
  install ( FILES ${MSVC_PYTHON_EXECUTABLE_DIR}/python${Python_VERSION_MAJOR}${Python_VERSION_MINOR}.dll
            ${Python_EXECUTABLE} ${Python_W_EXECUTABLE}
            DESTINATION bin )
  # Python >= 3 has an minimal API DLL too
  if ( EXISTS ${MSVC_PYTHON_EXECUTABLE_DIR}/python${Python_VERSION_MAJOR}.dll )
    install ( FILES ${MSVC_PYTHON_EXECUTABLE_DIR}/python${Python_VERSION_MAJOR}.dll
              DESTINATION bin )
  endif()
endif()
