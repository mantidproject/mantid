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
#
function (find_pyqt major_version)
  message(STATUS "looking in ${CMAKE_MODULE_PATH}")
  if (EXISTS "${CMAKE_MODULE_PATH}/FindPyQt.py")
    set (_find_pyqt_py "${CMAKE_MODULE_PATH}/FindPyQt.py")
  else()
    find_file (_find_pyqt_py FindPyQt.py PATHS ${CMAKE_MODULE_PATH} )
  endif()

  if (NOT EXISTS ${_find_pyqt_py})
    message(FATAL_ERROR "Failed to find FindPyQt.py in \"${CMAKE_MODULE_PATH}\"")
  endif()
  execute_process (COMMAND ${PYTHON_EXECUTABLE} ${_find_pyqt_py} ${major_version}
    OUTPUT_VARIABLE _pyqt_config ERROR_VARIABLE _pyqt_config_err)
  if (_pyqt_config AND NOT _pyqt_config_err)
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

    if (NOT EXISTS "${PYQT${major_version}_SIP_DIR}/QtCore/QtCoremod.sip")
      message (FATAL_ERROR "Unable to find QtCore/QtCoremod.sip in ${PYQT${major_version}_SIP_DIR}. PyQt sip files are missing."
        "FindPyQt.py config gave:\n${_pyqt_config}")
    endif()
  else ()
    message (FATAL_ERROR "Error encountered while determining PyQt confguration:\n${_pyqt_config_err}")
  endif()

  find_package_handle_standard_args( PyQt${major_version}
    REQUIRED_VARS
       PYQT${major_version}_VERSION_STR PYQT${major_version}_VERSION PYQT${major_version}_VERSION_TAG
      PYQT${major_version}_SIP_DIR PYQT${major_version}_SIP_FLAGS PYQT${major_version}_PYUIC
    VERSION_VAR
      PYQT${major_version}_VERSION_STR
  )

endfunction (find_pyqt)
