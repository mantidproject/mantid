####################################################################################
# PythonInterface sub modules 
####################################################################################

# Set the root directory for the package
set ( PYTHON_PKG_ROOT ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}/mantid )

####################################################################################
# Root module 
####################################################################################
# The destination directory, used by sub modules
set ( OUTPUT_DIR ${PYTHON_PKG_ROOT} )

# Select the pure Python files to be included in the distribution
set ( PY_FILES
  __init__.py
  simpleapi.py
)

copy_python_files_to_dir ( "${PY_FILES}" ${CMAKE_CURRENT_SOURCE_DIR} ${OUTPUT_DIR} 
                           PYTHON_PKGROOT_INSTALL_FILES )

if ( MSVC )
  # On windows copy over the python wrapper to the build directory (hang it off kernel build)
  add_custom_command ( OUTPUT ${PYTHON_EXE_WRAPPER}
                       COMMAND ${CMAKE_COMMAND} ARGS -E copy_if_different
                       ${PYTHON_EXE_WRAPPER_SRC} ${PYTHON_EXE_WRAPPER} )
endif()

# There is no library target for the top level package just the python files
add_custom_target ( PythonModule DEPENDS ${PYTHON_PKGROOT_INSTALL_FILES} ${PYTHON_EXE_WRAPPER} SOURCES ${PY_FILES} ${PYTHON_EXE_WRAPPER_SRC} )
set_property ( TARGET PythonModule PROPERTY FOLDER "MantidFramework/Python" )

####################################################################################
# Sub modules 
####################################################################################
add_subdirectory ( kernel )
add_dependencies ( PythonKernelModule PythonModule ) # Ensure the module files are in place
add_subdirectory ( geometry )
add_subdirectory ( api )

# Create an overall target
add_custom_target ( PythonInterface DEPENDS PythonKernelModule PythonGeometryModule PythonAPIModule  )
set_property ( TARGET PythonInterface PROPERTY FOLDER "MantidFramework/Python" )

###########################################################################
# Installation settings
###########################################################################

# Pure Python files 
install ( FILES ${PY_FILES} DESTINATION ${BIN_DIR}/mantid )
