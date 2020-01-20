# ##############################################################################
# Configure required dependencies if necessary
# ##############################################################################
option(WITH_PYTHON3 "If true build against Python 3, else use Python 2" OFF)

if(MSVC)
  # Git LFS does not work properly with <= 1.9
  find_package(Git 1.9.5 REQUIRED)
  find_package(GitLFS REQUIRED)

  # Use ExternalProject functionality as it already knows how to do clone/update
  include(ExternalProject)
  set(EXTERNAL_ROOT
      ${PROJECT_SOURCE_DIR}/external
      CACHE PATH "Location to clone third party dependencies to"
  )
  set(THIRD_PARTY_GIT_URL
      "https://github.com/mantidproject/thirdparty-msvc2015.git"
  )
  set(THIRD_PARTY_GIT_SHA1 a7bd18f35c8d67e68c3a965a07057efa266fc7d7)
  set(THIRD_PARTY_DIR ${EXTERNAL_ROOT}/src/ThirdParty)
  # Generates a script to do the clone/update in tmp
  set(_project_name ThirdParty)
  externalproject_add(
    ${_project_name}
    PREFIX ${EXTERNAL_ROOT}
    GIT_REPOSITORY ${THIRD_PARTY_GIT_URL}
    GIT_TAG ${THIRD_PARTY_GIT_SHA1}
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
    TEST_COMMAND ""
  )
  set_target_properties(
    ${_project_name} PROPERTIES EXCLUDE_FROM_DEFAULT_BUILD 1 EXCLUDE_FROM_ALL 1
  )

  # Do fetch/update now as we need the dependencies to configure
  set(_tmp_dir ${EXTERNAL_ROOT}/tmp)
  if(NOT EXISTS ${THIRD_PARTY_DIR}/.git)
    message(STATUS "Fetching third party dependencies")
    # As of git lfs 1.02 the default 'git checkout' behaviour is very slow for a
    # large amount of data. Running the 'git lfs fetch' command however produces
    # better suitable performance as it downloads everything in parallel. We
    # there for first clone the bare repository containing the data pointers and
    # update them manually see https://github.com/github/git-lfs/issues/376 for
    # more information
    set(ENV{GIT_LFS_SKIP_SMUDGE} 1)
    execute_process(
      COMMAND ${CMAKE_COMMAND} ARGS -P
              ${_tmp_dir}/${_project_name}-gitclone.cmake
      RESULT_VARIABLE error_code
    )
    if(error_code)
      message(
        FATAL_ERROR "Failed to clone repository: '${THIRD_PARTY_GIT_URL}'"
      )
    endif()
    unset(ENV{GIT_LFS_SKIP_SMUDGE})
    # Fetch the binary data
    execute_process(
      COMMAND ${GIT_EXECUTABLE} lfs fetch
      WORKING_DIRECTORY ${THIRD_PARTY_DIR}
      RESULT_VARIABLE error_code
    )
    if(error_code)
      message(
        FATAL_ERROR
          "Failed to download third party binary data. Check your network connection"
      )
    endif()
    # Checkout the data from the index to the working directory
    execute_process(
      COMMAND ${GIT_EXECUTABLE} lfs checkout
      WORKING_DIRECTORY ${THIRD_PARTY_DIR}
      RESULT_VARIABLE error_code
    )
  else()
    message(STATUS "Updating third party dependencies")
    # Assume the updates are small & don't run git lfs fetch
    execute_process(
      COMMAND ${CMAKE_COMMAND} ARGS -P
              ${_tmp_dir}/${_project_name}-gitupdate.cmake
      RESULT_VARIABLE error_code
    )
    if(error_code)
      message(
        FATAL_ERROR "Failed to update repository: '${THIRD_PARTY_GIT_URL}'"
      )
    endif()
  endif()
  unset(_tmp_dir)

  # Print out where we are looking for 3rd party stuff
  if(WITH_PYTHON3)
    set(PYTHON_VERSION_MAJOR 3)
    set(PYTHON_VERSION_MINOR 8)
  else()
    set(PYTHON_VERSION_MAJOR 2)
    set(PYTHON_VERSION_MINOR 7)
  endif()
  # used in later parts for MSVC to bundle Python
  set(MSVC_PYTHON_EXECUTABLE_DIR ${THIRD_PARTY_DIR}/lib/python${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR})
  set(PYTHON_EXECUTABLE ${MSVC_PYTHON_EXECUTABLE_DIR}/python.exe CACHE FILEPATH "Location of python executable" FORCE)
  set(PYTHONW_EXECUTABLE "${MSVC_PYTHON_EXECUTABLE_DIR}/pythonw.exe" CACHE FILEPATH
      "The location of the pythonw executable. This suppresses the new terminal window on startup" FORCE )
  set(THIRD_PARTY_BIN
      "${THIRD_PARTY_DIR}/bin;${THIRD_PARTY_DIR}/lib/qt4/bin;${THIRD_PARTY_DIR}/lib/qt5/bin;${MSVC_PYTHON_EXECUTABLE_DIR}"
  )
  message(STATUS "Third party dependencies are in ${THIRD_PARTY_DIR}")
  # Add to the path so that cmake can configure correctly without the user
  # having to do it
  set(ENV{PATH} "${THIRD_PARTY_BIN};$ENV{PATH}")

  # Set variables to help CMake find components
  set(CMAKE_INCLUDE_PATH "${THIRD_PARTY_DIR}/include")
  include_directories(${THIRD_PARTY_DIR}/include)
  set(CMAKE_LIBRARY_PATH "${THIRD_PARTY_DIR}/lib")
  set(CMAKE_PREFIX_PATH "${THIRD_PARTY_DIR};${THIRD_PARTY_DIR}/lib/qt4")
  set(BOOST_INCLUDEDIR "${CMAKE_INCLUDE_PATH}")
  set(BOOST_LIBRARYDIR "${CMAKE_LIBRARY_PATH}")
  set(Boost_NO_SYSTEM_PATHS TRUE)
else()
  unset(PYTHON_EXECUTABLE CACHE)
  if(WITH_PYTHON3)
    find_program(PYTHON_EXECUTABLE python3)
    if(NOT PYTHON_EXECUTABLE)
      message(FATAL_ERROR "Unable to find python3 executable")
    endif()
  else()
    find_program(PYTHON_EXECUTABLE python)
    if(NOT PYTHON_EXECUTABLE)
      message(FATAL_ERROR "Unable to find python executable")
    endif()
  endif()

  if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    # Homebrew adds qt4 here and we require it to be unlinked from /usr/local to
    # avoid qt4/qt5 cross talk
    list(APPEND CMAKE_PREFIX_PATH /usr/local/opt/qt@4)
  endif()
  find_package(Git)
endif()

# Clean out python variables set from a previous build so they can be
# rediscovered again
function(unset_cached_python_variables)
  foreach(
    _var
    PYTHON_INCLUDE_DIR
    PYTHON_LIBRARY
    PYTHON_NUMPY_INCLUDE_DIR
    SIP_INCLUDE_DIR
    PYQT4_PYUIC
    PYQT4_SIP_DIR
    PYQT4_SIP_FLAGS
    PYQT4_VERSION
    PYQT4_VERSION_STR
    PYQT4_VERSION_TAG
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
find_package(PythonInterp REQUIRED)
message(STATUS "Python version is " ${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR}.${PYTHON_VERSION_PATCH})
# Ensure FindPythonLibs finds the correct libraries
set(Python_ADDITIONAL_VERSIONS ${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR})

# Handle switching between previously configured Python 2 & Python 3 builds
if(PYTHON_INCLUDE_DIR AND NOT PYTHON_INCLUDE_DIR MATCHES ".*${PYTHON_VERSION_MAJOR}\.${PYTHON_VERSION_MINOR}.*" )
  message(STATUS "Python version has changed. Clearing previous Python configuration." )
  unset_cached_python_variables()
endif()
