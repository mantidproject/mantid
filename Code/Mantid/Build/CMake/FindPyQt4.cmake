# Find PyQt4
# ~~~~~~~~~~
# Copyright (c) 2007-2008, Simon Edwards <simon@simonzone.com>
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.
#
# PyQt4 website: http://www.riverbankcomputing.co.uk/pyqt/index.php
#
# Find the installed version of PyQt4. FindPyQt4 should only be called after
# Python has been found.
#
# This file defines the following variables:
#
# PYQT4_VERSION - The version of PyQt4 found expressed as a 6 digit hex number
#     suitable for comparision as a string
#
# PYQT4_VERSION_STR - The version of PyQt4 as a human readable string.
#
# PYQT4_VERSION_TAG - The PyQt version tag using by PyQt's sip files.
#
# PYQT4_SIP_DIR - The directory holding the PyQt4 .sip files.
#
# PYQT4_SIP_FLAGS - The SIP flags used to build PyQt.

IF(EXISTS PYQT4_VERSION)
  # Already in cache, be silent
  SET(PYQT4_FOUND TRUE)
ELSE(EXISTS PYQT4_VERSION)

  FIND_FILE(_find_pyqt_py FindPyQt.py PATHS ${CMAKE_MODULE_PATH})

  EXECUTE_PROCESS(COMMAND ${PYTHON_EXECUTABLE} ${_find_pyqt_py} OUTPUT_VARIABLE pyqt_config)
  IF( NOT ${pyqt_config} MATCHES "Traceback" )
    STRING(REGEX REPLACE "^pyqt_version:([^\n]+).*$" "\\1" PYQT4_VERSION ${pyqt_config})
    STRING(REGEX REPLACE ".*\npyqt_version_str:([^\n]+).*$" "\\1" PYQT4_VERSION_STR ${pyqt_config})
    STRING(REGEX REPLACE ".*\npyqt_version_tag:([^\n]+).*$" "\\1" PYQT4_VERSION_TAG ${pyqt_config})
    STRING(REGEX REPLACE ".*\npyqt_sip_dir:([^\n]+).*$" "\\1" PYQT4_SIP_DIR ${pyqt_config})
    STRING(REGEX REPLACE ".*\npyqt_sip_flags:([^\n]+).*$" "\\1" PYQT4_SIP_FLAGS ${pyqt_config})

    SET(PYQT4_FOUND TRUE)
  ENDIF()

  include ( FindPackageHandleStandardArgs )
  find_package_handle_standard_args ( PyQt4 DEFAULT_MSG PYQT4_VERSION )

ENDIF(EXISTS PYQT4_VERSION)
