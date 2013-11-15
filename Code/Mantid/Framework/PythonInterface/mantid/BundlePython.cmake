###############################################################################
# BundlePython.cmake
#
# This file contains the code to bundle the Python distribution from
# ThirdParty/PythonXX.
#
###############################################################################

if( WIN32 )
  #####################################################################
  # Bundled Python for dev build
  #####################################################################
 
  ## Python package directories ##
  file(TO_NATIVE_PATH ${CMAKE_LIBRARY_PATH}/Python27 SRC_NATIVE)
  if ( MSVC_IDE )
    file(TO_NATIVE_PATH ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR} BIN_NATIVE)
    add_custom_command( TARGET ${PYBUNDLE_POST_TARGET} POST_BUILD
                        COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/setup_dev_bundle.bat 
                        ARGS ${CMAKE_CFG_INTDIR} ${SRC_NATIVE} ${BIN_NATIVE} )
  else()
    file(TO_NATIVE_PATH ${CMAKE_LIBRARY_OUTPUT_DIRECTORY} BIN_NATIVE)
    add_custom_command( TARGET ${PYBUNDLE_POST_TARGET} POST_BUILD
                        COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/setup_dev_bundle.bat 
                        ARGS ${CMAKE_BUILD_TYPE} ${SRC_NATIVE} ${BIN_NATIVE} )
  endif()
  
  #####################################################################
  # Bundle for package
  #####################################################################
  install ( DIRECTORY ${CMAKE_LIBRARY_PATH}/Python27/DLLs DESTINATION bin PATTERN ".svn" EXCLUDE PATTERN ".git" EXCLUDE
                                                                          PATTERN "*_d.pyd" EXCLUDE PATTERN "*_d.dll" EXCLUDE )
  install ( DIRECTORY ${CMAKE_LIBRARY_PATH}/Python27/Lib DESTINATION bin PATTERN ".svn" EXCLUDE PATTERN ".git" EXCLUDE PATTERN "*_d.pyd" EXCLUDE )
  install ( DIRECTORY ${CMAKE_LIBRARY_PATH}/Python27/Scripts DESTINATION bin PATTERN ".svn" EXCLUDE PATTERN ".git" EXCLUDE PATTERN "*_d.py" EXCLUDE )
  install ( FILES ${PY_DLL_PREFIX}${PY_DLL_SUFFIX_RELEASE} ${PYTHON_EXECUTABLE} ${PYTHONW_EXECUTABLE} DESTINATION bin )
endif( WIN32 )
