# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI, NScD Oak Ridge National Laboratory, European
# Spallation Source, Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS SPDX - License - Identifier:
# GPL - 3.0 +

#[=======================================================================[.rst:
FindPyQt6
---------

Find the installed version of the PyQt6 libraries.
FindPyQt6 should be called after Python has been found.

PyQt6 website: https://www.riverbankcomputing.com/static/Docs/PyQt6/

Unlike PyQt5, PyQt6 only ships the sip >= v6 (sip-build) build system, so the
legacy ``PYQT6_SIP_FLAGS`` / ``PYQT6_VERSION_TAG`` values (consumed only by the
old ``sip -c`` path) are not produced here. PyQt6.QtCore also no longer exposes
``PYQT_CONFIGURATION``.

Result Variables
^^^^^^^^^^^^^^^^

``PYQT6_FOUND``
  True if PyQt6 has been found.

``PYQT6_VERSION``
  The version of PyQt6 found expressed as a 6 digit hex number
  suitable for comparison as a string

``PYQT6_VERSION_STR``
  The version of PyQt6 as a human readable string.

``PYQT6_SIP_DIR``
  The directory holding the PyQt6 .sip files.

``PYQT6_SIP_ABI_VERSION``
  The version of the sip ABI used to build against in the >=v6 build system

``PYQT6_PYUIC``
  Location of the pyuic script (best-effort; not required).

#]=======================================================================]

if(NOT PyQt6_FOUND)

  set(_command
      "
import os
import site
import sys

pyqt_name = 'PyQt6'
qtcore = __import__(f'{pyqt_name}.QtCore', globals(), locals(), ['QtCore'], 0)
version_hex = qtcore.PYQT_VERSION
version_str = qtcore.PYQT_VERSION_STR

try:
    uic = __import__(pyqt_name + '.uic', globals(), locals(), ['uic'], 0)
    pyuic_path = os.path.join(os.path.dirname(uic.__file__), 'pyuic.py')
except Exception:
    pyuic_path = ''

# The order of this list must match the order the variables are extracted below
lines = [
sys.prefix,
'|'.join(site.getsitepackages()),
f'{version_hex:06x}',
f'{version_str}',
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
  list(GET _pyqt_config 2 _pyqt6_version)
  list(GET _pyqt_config 3 _pyqt6_version_str)
  list(GET _pyqt_config 4 _pyqt6_pyuic)

  # Set caches
  set(PYQT6_VERSION
      ${_pyqt6_version}
      CACHE STRING "PyQt6's version as a 6-digit hexadecimal number" FORCE
  )
  set(PYQT6_VERSION_STR
      ${_pyqt6_version_str}
      CACHE STRING "PyQt6's version as a human-readable string" FORCE
  )
  set(PYQT6_PYUIC
      ${_pyqt6_pyuic}
      CACHE STRING "Location of the pyuic script" FORCE
  )

else()
  message(FATAL_ERROR "Error encountered while determining PyQt6 configuration (exit code ${_result}):\n"
                      "stdout: ${_pyqt_config}\n" "stderr: ${_error}"
  )
endif()

# If the user has provided ``PyQt6_ROOT_DIR``, use it.  Choose items found at this location over system locations.
if(EXISTS "$ENV{PyQt6_ROOT_DIR}")
  file(TO_CMAKE_PATH "$ENV{PyQt6_ROOT_DIR}" PyQt6_ROOT_DIR)
  set(PyQt6_ROOT_DIR
      "${PyQt6_ROOT_DIR}"
      CACHE PATH "Prefix for PyQt6 installation."
  )
endif()

# =============================================================================
# Set SIP_DIR. The find_path calls will prefer custom locations over standard locations (HINTS). Common locations
# followed by OS-dependent versions
list(APPEND _sip_hints ${_python_site_packages})
list(APPEND _sip_suffixes "share/sip/PyQt6" "PyQt6/bindings")
if(WIN32)
  list(APPEND _sip_hints ${_python_prefix})
  list(APPEND _sip_suffixes "sip/PyQt6")
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  list(APPEND _sip_hints "/usr/local/opt")
  list(APPEND _sip_suffixes "share/PyQt6" "pyqt/share/sip/Qt6" "mantid-pyqt6/share/sip/Qt6")
else()
  list(APPEND _sip_hints ${_python_prefix}/share)
  list(APPEND _sip_suffixes "python${Python_MAJOR_VERSION}${Python_MINOR_VERSION}-sip/PyQt6"
       "python${Python_MAJOR_VERSION}-sip/PyQt6" "sip/PyQt6"
  )
endif()

if(NOT EXISTS "${PYQT6_SIP_DIR}/QtCore/QtCoremod.sip")
  unset(PYQT6_SIP_DIR CACHE) # force reset from a previous cache value that has moved
endif()
find_path(
  PYQT6_SIP_DIR
  NAMES QtCore/QtCoremod.sip
  HINTS ${PyQt6_ROOT_DIR} ${_sip_hints}
  PATH_SUFFIXES ${_sip_suffixes}
)

# PyQt6 (>=6.5, pyqt6-sip 13.x) compiles against v13 of the sip ABI
set(PYQT6_SIP_ABI_VERSION
    13
    CACHE STRING "The sip ABI used to compile PyQt6" FORCE
)

if(NOT EXISTS "${PYQT6_SIP_DIR}/QtCore/QtCoremod.sip")
  message(FATAL_ERROR "Unable to find QtCore/QtCoremod.sip in ${PYQT6_SIP_DIR}. PyQt sip files are missing.")
endif()

include(FindPackageHandleStandardArgs)
# PYQT6_PYUIC is intentionally not required - PyQt6 may not ship pyuic in all layouts
find_package_handle_standard_args(PyQt6 PYQT6_VERSION PYQT6_VERSION_STR PYQT6_SIP_DIR PYQT6_SIP_ABI_VERSION)

mark_as_advanced(PYQT6_VERSION PYQT6_VERSION_STR PYQT6_SIP_DIR PYQT6_PYUIC PYQT6_SIP_ABI_VERSION)
