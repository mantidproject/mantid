###############################################################################
# BundlePython.cmake
#
# This file contains the code to bundle the Python distribution from
# ThirdParty/PythonXX.
#
###############################################################################

if( WIN32 )
  #####################################################################
  # Bundled for dev build
  #####################################################################

  # Use xcopy to as it has the ability to copy directories but only those files that
  # have been updated. It requires native paths though.
  file(TO_NATIVE_PATH ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR} BIN_CFG_NATIVE)
  add_custom_command( TARGET ${PYBUNDLE_POST_TARGET} POST_BUILD
                      COMMAND ${CMAKE_COMMAND} ARGS -E echo
                      "Setting up bundled Python installation"
  )
  foreach ( PYDIR ${PY_DIST_DIRS} )
    string(REGEX REPLACE ".*/(.*)" "\\1" PYDIR_RELATIVE ${PYDIR})
    file(TO_NATIVE_PATH ${PYDIR} PYDIR_NATIVE)
    add_custom_command( TARGET ${PYBUNDLE_POST_TARGET} POST_BUILD
                        COMMAND ${CMAKE_COMMAND} ARGS -E echo
                        "Updating ${PYDIR} directory"
                        COMMAND XCOPY ARGS /Y /E /F /D /I 
                        ${PYDIR_NATIVE} ${BIN_CFG_NATIVE}\\${PYDIR_RELATIVE}
                      )
  endforeach ( PYDIR )
  set ( BIN_CFG_NATIVE )
  set ( PYDIR_NATIVE )
  
  # Now the binary files
  # The mingw libs are required by the compiled fortran files to import correctly
  file ( GLOB MINGW_DLLS "${CMAKE_LIBRARY_PATH}/mingw/*.dll" )
  if ( MSVC_IDE )
    foreach ( TYPE DLL EXE EXEW )
      add_custom_command( TARGET ${PYBUNDLE_POST_TARGET} POST_BUILD 
                          COMMAND SET ARGS PY_${TYPE}_SUFFIX_RELEASE=${PY_${TYPE}_SUFFIX_RELEASE}
                          COMMAND SET ARGS PY_${TYPE}_SUFFIX_RELWITHDEBINFO=${PY_${TYPE}_SUFFIX_RELWITHDEBINFO}
                          COMMAND SET ARGS PY_${TYPE}_SUFFIX_MINSIZEREL=${PY_${TYPE}_SUFFIX_MINSIZEREL}
                          COMMAND SET ARGS PY_${TYPE}_SUFFIX_DEBUG=${PY_${TYPE}_SUFFIX_DEBUG}
                          COMMAND ${CMAKE_COMMAND} ARGS -E copy_if_different
                          ${PY_${TYPE}_PREFIX}%PY_${TYPE}_SUFFIX_${CMAKE_CFG_INTDIR}% ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}
                        )
    endforeach ( TYPE )
    foreach ( DLL_FILE ${MINGW_DLLS} )
      add_custom_command( TARGET ${PYBUNDLE_POST_TARGET} POST_BUILD
                          COMMAND ${CMAKE_COMMAND} ARGS -E copy_if_different
                          ${DLL_FILE} ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR} )
    endforeach( DLL_FILE )
  else () # Need to do things slightly differently for nmake
    foreach ( TYPE DLL EXE EXEW )
      add_custom_command( TARGET ${PYBUNDLE_POST_TARGET} POST_BUILD 
                          COMMAND SET ARGS PY_${TYPE}_SUFFIX_RELEASE=${PY_${TYPE}_SUFFIX_RELEASE}
                          COMMAND SET ARGS PY_${TYPE}_SUFFIX_RELWITHDEBINFO=${PY_${TYPE}_SUFFIX_RELWITHDEBINFO}
                          COMMAND SET ARGS PY_${TYPE}_SUFFIX_MINSIZEREL=${PY_${TYPE}_SUFFIX_MINSIZEREL}
                          COMMAND SET ARGS PY_${TYPE}_SUFFIX_DEBUG=${PY_${TYPE}_SUFFIX_DEBUG}
                          COMMAND ${CMAKE_COMMAND} ARGS -E copy_if_different
                          ${PY_${TYPE}_PREFIX}%PY_${TYPE}_SUFFIX_${CMAKE_BUILD_TYPE}% ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
                        )
    endforeach ( TYPE )
    foreach ( DLL_FILE MINGW_DLLS )
      add_custom_command( TARGET ${PYBUNDLE_POST_TARGET} POST_BUILD
                          COMMAND ${CMAKE_COMMAND} ARGS -E copy_if_different
                          ${DLL_FILE} ${CMAKE_LIBRARY_OUTPUT_DIRECTORY} )
    endforeach( DLL_FILE )
  endif ()
  
  #####################################################################
  # Bundle for package generation
  #####################################################################
  install ( DIRECTORY ${CMAKE_LIBRARY_PATH}/Python27/DLLs DESTINATION bin PATTERN ".svn" EXCLUDE PATTERN ".git" EXCLUDE
                                                                          PATTERN "*_d.pyd" EXCLUDE PATTERN "*_d.dll" EXCLUDE )
  install ( DIRECTORY ${CMAKE_LIBRARY_PATH}/Python27/Lib DESTINATION bin PATTERN ".svn" EXCLUDE PATTERN ".git" EXCLUDE PATTERN "*_d.pyd" EXCLUDE )
  install ( DIRECTORY ${CMAKE_LIBRARY_PATH}/Python27/Scripts DESTINATION bin PATTERN ".svn" EXCLUDE PATTERN ".git" EXCLUDE PATTERN "*_d.py" EXCLUDE )
  install ( FILES ${PY_DLL_PREFIX}${PY_DLL_SUFFIX_RELEASE} ${PYTHON_EXECUTABLE} ${PYTHONW_EXECUTABLE} DESTINATION bin )
endif( WIN32 )
