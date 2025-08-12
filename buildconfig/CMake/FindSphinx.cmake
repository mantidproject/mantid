#
# This module attempts to find the Python Sphinx documentation generator
#
# If the module is found the following variables are set: SPHINX_FOUND SPHINX_PACKAGE_DIR SPHINX_VERSION
#
# =============================================================
# main()
# =============================================================

# since sphinx version 1.3
set(SPHINX_MAIN "sphinx") # sphinx can be called as a module

# ask sphinx for its version
execute_process(
  COMMAND ${Python_EXECUTABLE} -m ${SPHINX_MAIN} --version
  OUTPUT_VARIABLE sphinx_output
  RESULT_VARIABLE sphinx_result
  OUTPUT_STRIP_TRAILING_WHITESPACE
)
if(${sphinx_result} STREQUAL "0")
  string(REGEX MATCH "([0-9]+\\.[0-9]+\\.?[0-9]*)" SPHINX_VERSION "${sphinx_output}")
else()
  message(WARN "failed to run ${Python_EXECUTABLE} -m ${SPHINX_MAIN} --version returned ${sphinx_result}")
endif()

# standard cmake registration method
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
  Sphinx
  REQUIRED_VARS SPHINX_VERSION
  VERSION_VAR SPHINX_VERSION
)

# =============================================================
# declare arguments for running sphinx
# =============================================================
# since sphinx version 1.3
set(SPHINX_NOCOLOR "--no-color")
# since sphinx version 2.0
set(SPHINX_KEEPGOING "--keep-going")
