# ~~~
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
# ~~~
function(find_pyqt major_version)
  find_file(_find_pyqt_py FindPyQt.py PATHS ${CMAKE_MODULE_PATH})
  if(NOT EXISTS ${_find_pyqt_py})
    message(FATAL_ERROR "Failed to find FindPyQt.py in \"${CMAKE_MODULE_PATH}\"")
  endif()
  # Dump config to stdout and parse out the content
  execute_process(
    COMMAND ${Python_EXECUTABLE} ${_find_pyqt_py} ${major_version}
    OUTPUT_VARIABLE _pyqt_config
    ERROR_VARIABLE _pyqt_config_err
  )
  if(_pyqt_config AND NOT _pyqt_config_err)
    string(REGEX MATCH "^pyqt_version:([^\n]+).*$" _dummy ${_pyqt_config})
    set(PYQT${major_version}_VERSION
        "${CMAKE_MATCH_1}"
        CACHE STRING "PyQt${major_version}'s version as a 6-digit hexadecimal number" FORCE
    )

    string(REGEX MATCH ".*\npyqt_version_str:([^\n]+).*$" _dummy ${_pyqt_config})
    set(PYQT${major_version}_VERSION_STR
        "${CMAKE_MATCH_1}"
        CACHE STRING "PyQt${major_version}'s version as a human-readable string" FORCE
    )

    string(REGEX MATCH ".*\npyqt_version_tag:([^\n]+).*$" _dummy ${_pyqt_config})
    set(PYQT${major_version}_VERSION_TAG
        "${CMAKE_MATCH_1}"
        CACHE STRING "The Qt version tag used by PyQt${major_version}'s .sip files" FORCE
    )

    string(REGEX MATCH ".*\npyqt_sip_dir:([^\n]+).*$" _dummy ${_pyqt_config})
    set(PYQT${major_version}_SIP_DIR
        "${CMAKE_MATCH_1}"
        CACHE PATH "The base directory where PyQt${major_version}'s .sip files are installed" FORCE
    )

    string(REGEX MATCH ".*\npyqt_sip_flags:([^\n]+).*$" _dummy ${_pyqt_config})
    set(PYQT${major_version}_SIP_FLAGS
        "${CMAKE_MATCH_1}"
        CACHE STRING "The SIP flags used to build PyQt${major_version}" FORCE
    )

    string(REGEX MATCH ".*\npyqt_pyuic:([^\n]+).*$" _dummy ${_pyqt_config})
    set(PYQT${major_version}_PYUIC
        "${CMAKE_MATCH_1}"
        CACHE STRING "Location of the pyuic script" FORCE
    )

    if(NOT EXISTS "${PYQT${major_version}_SIP_DIR}/QtCore/QtCoremod.sip")
      message(
        FATAL_ERROR
          "Unable to find QtCore/QtCoremod.sip in ${PYQT${major_version}_SIP_DIR}. PyQt sip files are missing."
          "FindPyQt.py config gave:\n${_pyqt_config}"
      )
    endif()
  else()
    message(FATAL_ERROR "Error encountered while determining PyQt confguration:\n${_pyqt_config_err}")
  endif()

  # Set the sip ABI used for compiling PyQt. Required for the new sip-build system
  if(major_version EQUAL 5)
    set(PYQT5_SIP_ABI_VERSION
        12
        CACHE STRING "The sip ABI used to compile PyQt5" FORCE
    )
  elseif(major_version EQUAL 6)
    set(PYQT6_SIP_ABI_VERSION
        13
        CACHE STRING "The sip ABI used to compile PyQt6" FORCE
    )
  else()
    message(FATAL_ERROR "Unknown PyQt major version specified: ${major_version}. \
        This buildsystem only understands building against PyQt 5/6"
    )
  endif()

  # Set standard arguments for find_package
  find_package_handle_standard_args(
    PyQt${major_version}
    REQUIRED_VARS
      PYQT${major_version}_VERSION_STR
      PYQT${major_version}_VERSION
      PYQT${major_version}_VERSION_TAG
      PYQT${major_version}_SIP_DIR
      PYQT${major_version}_SIP_FLAGS
      PYQT${major_version}_PYUIC
      PYQT${major_version}_SIP_ABI_VERSION
    VERSION_VAR PYQT${major_version}_VERSION_STR
  )

endfunction(find_pyqt)
