# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI, NScD Oak Ridge
# National Laboratory, European Spallation Source, Institut Laue - Langevin &
# CSNS, Institute of High Energy Physics, CAS SPDX - License - Identifier: GPL -
# 3.0 +

#[=======================================================================[.rst:
FindPyQt5
---------

Find the installed version of the PyQt5 libraries.
FindPyQt5 should be called after Python has been found.

PyQt5 website: https://www.riverbankcomputing.com/static/Docs/PyQt5/

Result Variables
^^^^^^^^^^^^^^^^

This module define the following variables:

``PYQT5_FOUND``
  True if PyQt5 has been found.

``PYQT5_VERSION``
  The version of PyQt5 found expressed as a 6 digit hex number
  suitable for comparison as a string

``PYQT5_VERSION_STR``
  The version of PyQt5 as a human readable string.

``PYQT5_VERSION_TAG``
  The Qt5 version tag used by PyQt's sip files.

``PYQT5_SIP_DIR``
  The directory holding the PyQt5 .sip files.

``PYQT5_SIP_FLAGS``
  The SIP flags used to build PyQt5.

``PYQT5_SIP_ABI_VERSION``
  The version of the sip ABI used to build against in the >=v6 build system

#]=======================================================================]

if(NOT PyQt5_FOUND)

set(_command "
import argparse
import os
import pprint
import re
import site
import sys

# Regular expression to extract the Qt version tag
QT_TAG_RE = re.compile(r'Qt_\\d+_\\d+_\\d+')

pyqt_name = 'PyQt5'
qtcore = __import__(f'{pyqt_name}.QtCore', globals(), locals(), ['QtCore'], 0)
version_hex = qtcore.PYQT_VERSION
version_str = qtcore.PYQT_VERSION_STR
sip_flags = qtcore.PYQT_CONFIGURATION['sip_flags']

match = QT_TAG_RE.search(sip_flags)
qt_tag = match.group(0) if match else None

uic = __import__(pyqt_name + '.uic', globals(), locals(), ['uic'], 0)
pyuic_path = os.path.join(os.path.dirname(uic.__file__), 'pyuic.py')

lines = [
'pyqt_version:%06.x' % version_hex,
'pyqt_version_str:%s' % version_str,
'pyqt_version_tag:%s' % qt_tag,
'pyqt_sip_flags:%s' % sip_flags,
'pyqt_pyuic:%s' % pyuic_path,
 sys.prefix,
]

sys.stdout.write('\\n'.join(lines))
"
)

set(_command_python_paths "
import site
import sys

sys.stdout.write(\";\".join((
  sys.prefix,
  site.getsitepackages()[0]
)))
"
)

execute_process(COMMAND "${Python_EXECUTABLE}" -c "${_command}"
                OUTPUT_VARIABLE _pyqt_config
                RESULT_VARIABLE _result
                ERROR_VARIABLE _error)

execute_process(COMMAND "${Python_EXECUTABLE}" -c "${_command_python_paths}"
                OUTPUT_VARIABLE _python_paths
                RESULT_VARIABLE _result_python_paths
                ERROR_VARIABLE _error_python_paths)

endif()

if (_pyqt_config)
string (REGEX MATCH "^pyqt_version:([^\n]+).*$" _dummy ${_pyqt_config})
set (PYQT5_VERSION "${CMAKE_MATCH_1}" CACHE STRING "PyQt5's version as a 6-digit hexadecimal number" FORCE)

string (REGEX MATCH ".*\npyqt_version_str:([^\n]+).*$" _dummy ${_pyqt_config})
set (PYQT5_VERSION_STR "${CMAKE_MATCH_1}" CACHE STRING "PyQt5's version as a human-readable string" FORCE)

string (REGEX MATCH ".*\npyqt_version_tag:([^\n]+).*$" _dummy ${_pyqt_config})
set (PYQT5_VERSION_TAG "${CMAKE_MATCH_1}" CACHE STRING "The Qt version tag used by PyQt5's .sip files" FORCE)

string (REGEX MATCH ".*\npyqt_sip_flags:([^\n]+).*$" _dummy ${_pyqt_config})
set (PYQT5_SIP_FLAGS "${CMAKE_MATCH_1}" CACHE STRING "The SIP flags used to build PyQt5" FORCE)

string (REGEX MATCH ".*\npyqt_pyuic:([^\n]+).*$" _dummy ${_pyqt_config})
set (PYQT5_PYUIC "${CMAKE_MATCH_1}" CACHE STRING "Location of the pyuic script" FORCE)

else ()
message (FATAL_ERROR "Error encountered while determining PyQt confguration:\n${_pyqt_config_err} ${_error}")
endif()

if (_python_paths)
list(GET __python_paths 0 _item)
set(PYTHON_PREFIX "${_item}")

list(GET _python_paths 1 _item)
set(PYTHON_SITE_PACKAGES "${_item}")

endif()

# If the user has provided ``PyQt_ROOT_DIR``, use it.  Choose items found
# at this location over system locations.
if( EXISTS "$ENV{PyQt5_ROOT_DIR}" )
  file( TO_CMAKE_PATH "$ENV{PyQt5_ROOT_DIR}" PyQt5_ROOT_DIR )
  set( PyQt5_ROOT_DIR "${PyQt5_ROOT_DIR}" CACHE PATH "Prefix for PyQt5 installation." )
endif()

#=============================================================================
# Set SIP_DIR. The find_path calls will prefer custom
# locations over standard locations (HINTS).
if(WIN32)
  list(APPEND _sip_hints ${PYTHON_PREFIX})
  list(APPEND _sip_suffixes "share/sip/PyQt5" "sip/PyQt5")
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  list(APPEND _sip_suffixes "share/sip/PyQt5" "share/PyQt5" "PyQt5/bindings" "pyqt/share/sip/Qt5" "mantid-pyqt5/share/sip/Qt5")
  list(APPEND _sip_hints "/usr/local/opt" ${PYTHON_SITE_PACKAGES})
else()
  list(APPEND _sip_hints ${PYTHON_PREFIX}/share)
  list(APPEND _sip_suffixes "share/sip/PyQt5" "python${Python_MAJOR_VERSION}${Python_MINOR_VERSION}-sip/PyQt5" "python${Python_MAJOR_VERSION}-sip/PyQt5" "sip/PyQt5")
endif()

find_path(PYQT5_SIP_DIR
  NAMES QtCore/QtCoremod.sip
  HINTS ${PyQt_ROOT_DIR} ${_sip_hints}
  PATH_SUFFIXES ${_sip_suffixes}
)

message("Found PyQt sip dir ${PYQT5_SIP_DIR}")
if (NOT EXISTS "${PYQT5_SIP_DIR}/QtCore/QtCoremod.sip")
  message (FATAL_ERROR "Unable to find QtCore/QtCoremod.sip in ${PYQT5_SIP_DIR}. PyQt sip files are missing.")
endif()

include ( FindPackageHandleStandardArgs )
find_package_handle_standard_args(PyQt5 PYQT5_VERSION PYQT5_VERSION_STR PYQT5_VERSION_TAG PYQT5_SIP_DIR PYQT5_SIP_FLAGS PYQT5_PYUIC)

mark_as_advanced ( PYQT5_VERSION
                   PYQT5_VERSION_STR
                   PYQT5_VERSION_TAG
                   PYQT5_SIP_DIR
                   PYQT5_SIP_FLAGS
                   PYQT5_PYUIC
                   )