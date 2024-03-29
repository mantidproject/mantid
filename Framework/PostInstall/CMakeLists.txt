# Defines a rule to compile all of the Python files that have been installed as part of the package.

# CMake seems to put all of the current-level install commands "before" any sub- directories that you have added, even
# if you add them before the install rule.

# To ensure that this script runs after everything is installed it is in this subdirectory so that it can be the last
# thing added. It is at the Framework level so that a "Framework Only" build is still valid.

# When building the package CPack installs into ${DESTDIR}${CMAKE_INSTALL_PREFIX} but this should be evaluated at CPack
# time so escape the $ to its written literally.
set(PACKAGE_ROOT \$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX})
set(EXCLUDE_REGEX ".*_template")

set(COMPILE_SCRIPT "message ( \"Byte-compiling Python in ${PACKAGE_ROOT}\")")
set(COMPILE_SCRIPT
    "${COMPILE_SCRIPT}\n  execute_process ( COMMAND ${Python_EXECUTABLE} -m compileall -q -j 0 -x \"${EXCLUDE_REGEX}\" \"${PACKAGE_ROOT}\" OUTPUT_QUIET ERROR_QUIET )"
)

if(NOT CONDA_BUILD)
  install(CODE ${COMPILE_SCRIPT})
endif()
