# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory UKRI, NScD Oak Ridge National Laboratory, European
# Spallation Source, Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS SPDX - License - Identifier:
# GPL - 3.0 +

#[=======================================================================[.rst:
FindSIP
-------

Find the installed version of the SIP Python binding library.
FindSIP should be called after Python has been found.
It supports both sip build systems in v6 and <= v5.

SIP website: http://www.riverbankcomputing.co.uk/sip/index.php

Result Variables
^^^^^^^^^^^^^^^^

All versions of sip define the following variables:

``SIP_FOUND``
  True if sip has been found.

``SIP_VERSION``
  The version of SIP found in ``X.Y.Z`` format

For versions 6 and this file additionally defines the following variables:

``SIP_BUILD_EXECUTABLE``
  The path to the sip-build executable

``SIP_MODULE_EXECUTABLE``
  The path to the sip-module executable

For versions prior to 5 this file additionally defines the following variables:

``SIP_EXECUTABLE``
Path and filename of the SIP command line executable.

``SIP_INCLUDE_DIR``
Directory holding the SIP C++ header file.

#]=======================================================================]
include(FindPackageHandleStandardArgs)

# First look for sip-build, indicating the newer v6 build system
find_program(SIP_BUILD_EXECUTABLE NAMES sip-build)

if(SIP_BUILD_EXECUTABLE)

  # version string
  execute_process(
    COMMAND ${SIP_BUILD_EXECUTABLE} --version
    OUTPUT_VARIABLE SIP_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )

  # module generator
  find_program(SIP_MODULE_EXECUTABLE NAMES sip-module)
  # pyproject.toml template
  set(SIP_PYPROJECT_TOML_TEMPLATE
      ${CMAKE_MODULE_PATH}/sip-templates/pyproject.toml.in
      CACHE FILEPATH ""
  )
  # project.py template
  set(SIP_PROJECT_PY_TEMPLATE
      ${CMAKE_MODULE_PATH}/sip-templates/project.py.in
      CACHE FILEPATH ""
  )

  # Set expected variables for find_package
  find_package_handle_standard_args(
    SIP
    REQUIRED_VARS SIP_BUILD_EXECUTABLE SIP_MODULE_EXECUTABLE SIP_PYPROJECT_TOML_TEMPLATE SIP_PROJECT_PY_TEMPLATE
    VERSION_VAR SIP_VERSION
  )
endif()

if(NOT SIP_FOUND)
  message(STATUS "looking for sip on older system")

  # Python development is required for the older system
  if(NOT Python_Development_FOUND)
    message(FATAL_ERROR "FindSIP requires find_package(Python) to be called first")
  endif()

  # Look for older sip build system. CentOS has this prefixed with python3
  find_program(SIP_EXECUTABLE NAMES python${Python_VERSION_MAJOR}-sip sip)

  # version string
  execute_process(
    COMMAND ${SIP_EXECUTABLE} -V
    OUTPUT_VARIABLE SIP_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )

  # header directory
  find_path(SIP_INCLUDE_DIR sip.h HINTS ${Python_INCLUDE_DIRS})

  # Set expected variables for find_package
  find_package_handle_standard_args(
    SIP
    REQUIRED_VARS SIP_EXECUTABLE SIP_INCLUDE_DIR
    VERSION_VAR SIP_VERSION
  )
endif()

if(NOT SIP_VERSION)
  message(WARNING "FindSIP failed to determine sip version")
endif()
