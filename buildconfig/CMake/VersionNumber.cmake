# Pull in the version number here for Kernel/src/MantidVersion.cpp.in . This roughly follows Semantic Versioning,
# https://semver.org, but will adhere to PEP440 https://peps.python.org/pep-0440/
#
# This implementation assumes the build is run from a Git repository. and uses Python package versioningit. It is
# extracted assuming it is set in the form:
#
# major.minor.patch(tweak(rcN))
#
# where tweak version and rcN number are optional. This assumes the rc value will not exist without a tweak version.
# Examples: `6.5.0`, `6.4.20220915.1529`, `6.4.0.1`, and `6.4.0.1rc2`.

# Use versioningit to compute version number to match conda-build
execute_process(
  COMMAND "${Python_EXECUTABLE}" -c "import setup;print(setup.get_version())"
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  OUTPUT_VARIABLE _version_str
  ERROR_VARIABLE _error
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Expect format defined as in comments above. Final optional capture group defines tweak version.
if(_version_str MATCHES "([0-9]+)\\.([0-9]+)\\.([A-za-z0-9]+)(\\..+)?")
  # Version components
  set(VERSION_MAJOR ${CMAKE_MATCH_1})
  set(VERSION_MINOR ${CMAKE_MATCH_2})
  set(VERSION_PATCH ${CMAKE_MATCH_3})
  set(VERSION_TWEAK ${CMAKE_MATCH_4})
else()
  message(FATAL_ERROR "Error extracting version elements from: ${_version_str}")
endif()
message(STATUS "Version: ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}${VERSION_TWEAK}")

# Revision information
execute_process(
  COMMAND ${GIT_EXECUTABLE} rev-parse HEAD
  OUTPUT_VARIABLE REVISION_FULL
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

execute_process(
  COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
  OUTPUT_VARIABLE REVISION_SHORT
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  OUTPUT_STRIP_TRAILING_WHITESPACE
)
# Historically the short revision has been prefixed with a 'g'
set(REVISION_SHORT "g${REVISION_SHORT}")

# Get the date of the last commit
execute_process(
  COMMAND ${GIT_EXECUTABLE} log -1 --format=format:%cD
  OUTPUT_VARIABLE REVISION_DATE
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)
string(SUBSTRING ${REVISION_DATE} 0 16 REVISION_DATE)
