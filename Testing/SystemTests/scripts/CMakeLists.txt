# Write out a files containing - the data search directories - the default save directory These will be read by the
# MantidConfig class to configure the properties file

# Data search directories
set(DATA_DIRS "")
# Input data
set(SYSTEM_TEST_DATA_DIR ${ExternalData_BINARY_ROOT}/Testing/Data/SystemTest)
list(APPEND DATA_DIRS ${SYSTEM_TEST_DATA_DIR})
list(APPEND DATA_DIRS ${SYSTEM_TEST_DATA_DIR}/INTER)
# Reference results
list(APPEND DATA_DIRS ${ExternalData_BINARY_ROOT}/Testing/SystemTests/tests/framework/reference)
# Unit tests
list(APPEND DATA_DIRS ${ExternalData_BINARY_ROOT}/Testing/Data/UnitTest)
# Doc tests
list(APPEND DATA_DIRS ${ExternalData_BINARY_ROOT}/Testing/Data/DocTest)

# Output to file If this is changed, make sure to update the .gitignore in this directory
set(DATA_DIRS_FILE ${CMAKE_CURRENT_LIST_DIR}/datasearch-directories.txt)
# The quotes are important as CMake translates the list to a semicolon-separated string
file(WRITE ${DATA_DIRS_FILE} "${DATA_DIRS}")

# Default save directory
set(SAVE_DIR ${CMAKE_CURRENT_BINARY_DIR})
set(SAVE_DIR_FILE ${CMAKE_CURRENT_LIST_DIR}/defaultsave-directory.txt)
file(WRITE ${SAVE_DIR_FILE} "${SAVE_DIR}")

# Configure files for developers running tests
set(SYSTEMTESTS_RUNNER_SCRIPT ${CMAKE_CURRENT_LIST_DIR}/runSystemTests.py)

get_property(_is_multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(_is_multi_config)
  set(MULTI_CONFIG_GENERATOR "true")
else()
  set(MULTI_CONFIG_GENERATOR "false")
endif()
if(MSVC)
  configure_file(${CMAKE_CURRENT_LIST_DIR}/systemtest.bat.in ${CMAKE_BINARY_DIR}/systemtest.bat @ONLY)
else()
  configure_file(${CMAKE_CURRENT_LIST_DIR}/systemtest.in ${CMAKE_BINARY_DIR}/systemtest @ONLY)
endif()
