# ######################################################################################################################
# Configure required dependencies if necessary
# ######################################################################################################################
# Find git for everything
if(WIN32)
  set(_git_requires 1.9.5)
endif()
find_package(Git ${_git_requires})

if(MSVC)
  # Print out where we are looking for 3rd party stuff
  set(Python_FIND_REGISTRY NEVER)
  # used in later parts for MSVC to bundle Python
  set(MSVC_PYTHON_EXECUTABLE_DIR $ENV{CONDA_PREFIX})
  set(THIRD_PARTY_BIN "$ENV{CONDA_PREFIX}/Library/bin;$ENV{CONDA_PREFIX}/Library/lib;${MSVC_PYTHON_EXECUTABLE_DIR}")
  # Add to the path so that cmake can configure correctly without the user having to do it
  set(ENV{PATH} "${THIRD_PARTY_BIN};$ENV{PATH}")
  # Set PATH for custom command or target build steps. Avoids the need to make external PATH updates
  set(CMAKE_MSVCIDE_RUN_PATH ${THIRD_PARTY_BIN})
endif()

# Clean out python variables set from a previous build so they can be rediscovered again
function(unset_cached_Python_variables)
  foreach(
    _var
    Python_INCLUDE_DIR
    Python_LIBRARY
    Python_NUMPY_INCLUDE_DIR
    SIP_INCLUDE_DIR
    PYQT5_PYUIC
    PYQT5_SIP_DIR
    PYQT5_SIP_FLAGS
    PYQT5_VERSION
    PYQT5_VERSION_STR
    PYQT5_VERSION_TAG
    PYRCC5_CMD
  )
    unset(${_var} CACHE)
  endforeach()
endfunction()

# Find python interpreter
set(MINIMUM_PYTHON_VERSION 3.11)
# If we are not building the mantid framework we don't need the numpy developer env
if(MANTID_FRAMEWORK_LIB STREQUAL "BUILD")
  find_package(Python ${MINIMUM_PYTHON_VERSION} REQUIRED COMPONENTS Interpreter Development NumPy)
else()
  find_package(Python ${MINIMUM_PYTHON_VERSION} REQUIRED COMPONENTS Interpreter Development)
endif()

# If anything external uses find_package(PythonInterp) then make sure it finds the correct version and executable
set(PYTHON_EXECUTABLE ${Python_EXECUTABLE})
set(Python_ADDITIONAL_VERSIONS ${Python_VERSION_MAJOR}.${Python_VERSION_MINOR})

# Search for the pythonw executable if it has not already been found Will only look in the folder containing the current
# python.exe
if(NOT Python_W_EXECUTABLE)
  get_filename_component(Python_Binary_Dir ${PYTHON_EXECUTABLE} DIRECTORY)
  find_program(
    Python_W_EXECUTABLE
    PATHS ${Python_Binary_Dir}
    NAMES pythonw
    NO_DEFAULT_PATH
  )
endif()

# Set a variable pointing to the relative location of the site packages directory w.r.t the prefix Used by install()
# calls to place the Python packages in the correct place
set(_code
    "import os.path as osp
import sys
import sysconfig as scfg
print(osp.relpath(scfg.get_path('purelib'), start=sys.prefix))
"
)
execute_process(
  COMMAND python -c ${_code}
  RESULT_VARIABLE _result
  OUTPUT_VARIABLE _output
  ERROR_VARIABLE _error
  OUTPUT_STRIP_TRAILING_WHITESPACE
)
if(_result EQUAL 0)
  if(WIN32)
    set(Python_SITELIB_RELPATH "..\\${_output}")
  else()
    set(Python_SITELIB_RELPATH ${_output})
  endif()
else()
  message(FATAL_ERROR "Error determining Python site packages location: ${_error}")
endif()
unset(_code)
unset(_result)
unset(_output)

# Handle switching between previously configured Python verions
if(Python_INCLUDE_DIR AND NOT Python_INCLUDE_DIR MATCHES ".*${Python_VERSION_MAJOR}\.${Python_VERSION_MINOR}.*")
  message(STATUS "Python version has changed. Clearing previous Python configuration.")
  unset_cached_python_variables()
endif()

# What version of setuptools are we using?
execute_process(
  COMMAND ${Python_EXECUTABLE} -c "import setuptools;print(setuptools.__version__)"
  RESULT_VARIABLE _setuptools_version_check_result
  OUTPUT_VARIABLE Python_SETUPTOOLS_VERSION
  ERROR_VARIABLE _setuptools_version_check_error
)
if(NOT _setuptools_version_check_result EQUAL 0)
  message(FATAL_ERROR "Unable to determine setuptools version:\n" "    ${_setuptools_version_check_error}")
endif()
