# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI, NScD Oak Ridge National Laboratory, European
# Spallation Source, Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS SPDX - License - Identifier:
# GPL - 3.0 +

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

  set(_command
      "
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

# The order of this list must match the order the variables are extracted below
lines = [
sys.prefix,
'|'.join(site.getsitepackages()),
f'{version_hex:06x}' ,
f'{version_str}',
f'{qt_tag}',
f'{sip_flags}',
f'{pyuic_path}',
]
# The ; is important as cmake will intrepet it as a list
sys.stdout.write(';'.join(lines))
"
  )

  execute_process(
    COMMAND "${Python_EXECUTABLE}" -c "${_command}"
    OUTPUT_VARIABLE _pyqt_config
    RESULT_VARIABLE _result
    ERROR_VARIABLE _error
  )

endif()

if(_pyqt_config)
  # Local variables, not cached
  list(GET _pyqt_config 0 _python_prefix)
  list(GET _pyqt_config 1 _python_site_packages)
  # replace | separator with ; to convert to CMake list
  string(REPLACE "|" ";" _python_site_packages ${_python_site_packages})
  list(GET _pyqt_config 2 _pyqt5_version)
  list(GET _pyqt_config 3 _pyqt5_version_str)
  list(GET _pyqt_config 4 _pyqt5_version_tag)
  list(GET _pyqt_config 5 _pyqt5_sip_flags)
  list(GET _pyqt_config 6 _pyqt5_pyuic)

  # Set caches
  set(PYQT5_VERSION
      ${_pyqt5_version}
      CACHE STRING "PyQt5's version as a 6-digit hexadecimal number" FORCE
  )
  set(PYQT5_VERSION_STR
      ${_pyqt5_version_str}
      CACHE STRING "PyQt5's version as a human-readable string" FORCE
  )
  set(PYQT5_VERSION_TAG
      ${_pyqt5_version_tag}
      CACHE STRING "The Qt version tag used by PyQt5's .sip files" FORCE
  )
  set(PYQT5_SIP_FLAGS
      ${_pyqt5_sip_flags}
      CACHE STRING "The SIP flags used to build PyQt5" FORCE
  )
  set(PYQT5_PYUIC
      ${_pyqt5_pyuic}
      CACHE STRING "Location of the pyuic script" FORCE
  )

else()
  message(FATAL_ERROR "Error encountered while determining PyQt confguration:\n${_pyqt_config_err} ${_error}")
endif()

# If the user has provided ``PyQt_ROOT_DIR``, use it.  Choose items found at this location over system locations.
if(EXISTS "$ENV{PyQt5_ROOT_DIR}")
  file(TO_CMAKE_PATH "$ENV{PyQt5_ROOT_DIR}" PyQt5_ROOT_DIR)
  set(PyQt5_ROOT_DIR
      "${PyQt5_ROOT_DIR}"
      CACHE PATH "Prefix for PyQt5 installation."
  )
endif()

# =============================================================================
# Set SIP_DIR. The find_path calls will prefer custom locations over standard locations (HINTS). Common locations
# followed by OS-dependent versions
list(APPEND _sip_hints ${_python_site_packages})
list(APPEND _sip_suffixes "share/sip/PyQt5" "PyQt5/bindings")
if(WIN32)
  list(APPEND _sip_hints ${_python_prefix})
  list(APPEND _sip_suffixes "sip/PyQt5")
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  list(APPEND _sip_hints "/usr/local/opt")
  list(APPEND _sip_suffixes "share/PyQt5" "pyqt/share/sip/Qt5" "mantid-pyqt5/share/sip/Qt5")
else()
  list(APPEND _sip_hints ${_python_prefix}/share)
  list(APPEND _sip_suffixes "python${Python_MAJOR_VERSION}${Python_MINOR_VERSION}-sip/PyQt5"
       "python${Python_MAJOR_VERSION}-sip/PyQt5" "sip/PyQt5"
  )
endif()

if(NOT EXISTS "${PYQT5_SIP_DIR}/QtCore/QtCoremod.sip")
  unset(PYQT5_SIP_DIR CACHE) # force reset from a previous cache value that has moved
endif()
find_path(
  PYQT5_SIP_DIR
  NAMES QtCore/QtCoremod.sip
  HINTS ${PyQt_ROOT_DIR} ${_sip_hints}
  PATH_SUFFIXES ${_sip_suffixes}
)

# PyQt5 compiles against v12 of the sip ABI
set(PYQT5_SIP_ABI_VERSION
    12
    CACHE STRING "The sip ABI used to compile PyQt5" FORCE
)

if(NOT EXISTS "${PYQT5_SIP_DIR}/QtCore/QtCoremod.sip")
  message(FATAL_ERROR "Unable to find QtCore/QtCoremod.sip in ${PYQT5_SIP_DIR}. PyQt sip files are missing.")
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
  PyQt5
  PYQT5_VERSION
  PYQT5_VERSION_STR
  PYQT5_VERSION_TAG
  PYQT5_SIP_DIR
  PYQT5_SIP_FLAGS
  PYQT5_PYUIC
  PYQT5_SIP_ABI_VERSION
)

mark_as_advanced(
  PYQT5_VERSION
  PYQT5_VERSION_STR
  PYQT5_VERSION_TAG
  PYQT5_SIP_DIR
  PYQT5_SIP_FLAGS
  PYQT5_PYUIC
  PYQT5_SIP_ABI_VERSION
)
