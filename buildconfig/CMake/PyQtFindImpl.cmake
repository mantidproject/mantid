# Implementation of searching for PyQt
#
# It contains a function parameterized by the major
# version of PyQt being searched.
#
# Search for the given version of PyQt. The function sets the
# following variables in the parent scope:
#
# PYQT${major_version}_VERSION - The version of PyQt found expressed as a 6 digit hex number
#     suitable for comparison as a string
#
# PYQT${major_version}_VERSION_STR - The version of PyQt as a human readable string.
#
# PYQT${major_version}_VERSION_TAG - The Qt version tag used by PyQt's sip files.
#
# PYQT${major_version}_SIP_DIR - The directory holding the PyQt .sip files.
#
# PYQT${major_version}_SIP_FLAGS - The SIP flags used to build PyQt.
function (find_pyqt major_version)
  if (PYQT${major_version}_VERSION)
    # Already in cache, be silent
    set (PYQT${major_version}_FOUND TRUE)
  else ()
      find_file (_find_pyqt_py FindPyQt.py PATHS ${CMAKE_MODULE_PATH})
      execute_process (COMMAND ${PYTHON_EXECUTABLE} ${_find_pyqt_py} ${major_version} OUTPUT_VARIABLE _pyqt_config)
      if(_pyqt_config)
        string (REGEX MATCH "^pyqt_version:([^\n]+).*$" _dummy ${_pyqt_config})
        set (PYQT${major_version}_VERSION "${CMAKE_MATCH_1}" CACHE STRING "PyQt${major_version}'s version as a 6-digit hexadecimal number")

        string (REGEX MATCH ".*\npyqt_version_str:([^\n]+).*$" _dummy ${_pyqt_config})
        set (PYQT${major_version}_VERSION_STR "${CMAKE_MATCH_1}" CACHE STRING "PyQt${major_version}'s version as a human-readable string")

        string (REGEX MATCH ".*\npyqt_version_tag:([^\n]+).*$" _dummy ${_pyqt_config})
        set (PYQT${major_version}_VERSION_TAG "${CMAKE_MATCH_1}" CACHE STRING "The Qt version tag used by PyQt${major_version}'s .sip files")

        string (REGEX MATCH ".*\npyqt_sip_dir:([^\n]+).*$" _dummy ${_pyqt_config})
        set (PYQT${major_version}_SIP_DIR "${CMAKE_MATCH_1}" CACHE PATH "The base directory where PyQt${major_version}'s .sip files are installed")

        string (REGEX MATCH ".*\npyqt_sip_flags:([^\n]+).*$" _dummy ${_pyqt_config})
        set (PYQT${major_version}_SIP_FLAGS "${CMAKE_MATCH_1}" CACHE STRING "The SIP flags used to build PyQt${major_version}")

        string (REGEX MATCH ".*\npyqt_pyuic:([^\n]+).*$" _dummy ${_pyqt_config})
        set (PYQT${major_version}_PYUIC "${CMAKE_MATCH_1}" CACHE STRING "Location of the pyuic script")

        if (NOT IS_DIRECTORY "${PYQT${major_version}_SIP_DIR}")
          message (WARNING "The base directory where PyQt${major_version}'s SIP files are installed could not be determined.\n"
            "This usually means the PyQt${major_version} development package is missing.\n")
        else()
          set (PYQT${major_version}_FOUND TRUE )
    endif()
  endif()

  if (PYQT${major_version}_FOUND)
    if (NOT PYQT${major_version}_FIND_QUIETLY)
      message (STATUS "Found PyQt${major_version} version: ${PYQT${major_version}_VERSION_STR}")
    endif ()
  else()
    if (PYQT${major_version}_FIND_REQUIRED)
      message (FATAL_ERROR "Could not find required PyQt${major_version}")
    endif()
  endif()

  endif()

endfunction (find_pyqt)
