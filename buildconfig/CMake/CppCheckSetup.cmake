find_package(Cppcheck)

if(CPPCHECK_EXECUTABLE)

  # We must export the compile commands for cppcheck to be able to check everything correctly
  set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
  set(CPPCHECK_BUILD_DIR "${CMAKE_BINARY_DIR}/cppcheck")
  file(MAKE_DIRECTORY "${CPPCHECK_BUILD_DIR}" "${CPPCHECK_BUILD_DIR}/cache")
  configure_file(
    ${CMAKE_SOURCE_DIR}/buildconfig/CMake/CppCheck_Suppressions.txt.in
    "${CPPCHECK_BUILD_DIR}/CppCheck_Suppressions.txt"
  )

  # Set up the standard arguments --inline-suppr appears to be ignored if --suppressions-list is specified. Cppcheck >=
  # 2.10 requires all "file" entries within a compile_commands.json to exist whereas previously only a warning was
  # emitted if an entry didn't exist. Files such as qrc_*, moc_* files from Qt are listed but don't exist as they are
  # made by build rules. We filter the raw cmake-generated compile_commands.json below before executing cppcheck.
  set(CPPCHECK_ARGS
      --enable=all
      # 2.12 has missingInclude switched on by default if running with multiple cores but it doesn't appear to work
      # correctly. It was flagging many stl and other external header files as missing.
      --disable=missingInclude
      # Adding qt here helps with mis-identifying Qt macros as unknownMacro defects.
      --library=qt
      --check-level=exhaustive
      --inline-suppr
      --max-configs=120
      --std=c++${CMAKE_CXX_STANDARD} # use the standard from cmake
      --cppcheck-build-dir="${CPPCHECK_BUILD_DIR}/cache"
      --suppressions-list="${CPPCHECK_BUILD_DIR}/CppCheck_Suppressions.txt"
      --project="${CMAKE_BINARY_DIR}/compile_commands_cppcheck.json"
      --checkers-report=${CMAKE_BINARY_DIR}/cppcheck_checkers_reports.txt
      -i"${CMAKE_BINARY_DIR}"
      # Force cppcheck to check when we use project-wide macros
      -DDLLExport=
      -DMANTID_ALGORITHMS_DLL=
      # Undefine problematic macros: Causes errors such as there was an internal error: bad macro syntax"
      -UQT_TESTCASE_BUILDDIR
  )

  set(_cppcheck_args "${CPPCHECK_ARGS}")
  list(APPEND _cppcheck_args ${CPPCHECK_TEMPLATE_ARG})
  if(CPPCHECK_NUM_THREADS GREATER 0)
    list(APPEND _cppcheck_args -j ${CPPCHECK_NUM_THREADS})
  endif(CPPCHECK_NUM_THREADS GREATER 0)

  # put the finishing bits on the final command call
  set(_cppcheck_xml_args)
  if(CPPCHECK_GENERATE_XML)
    list(
      APPEND
      _cppcheck_xml_args
      --xml
      --xml-version=2
      "${_cppcheck_source_dirs}"
      2>
      ${CMAKE_BINARY_DIR}/cppcheck.xml
    )
  else(CPPCHECK_GENERATE_XML)
    list(APPEND _cppcheck_xml_args "${_cppcheck_source_dirs}")
  endif(CPPCHECK_GENERATE_XML)

  # generate the target
  if(NOT TARGET cppcheck)
    add_custom_target(
      cppcheck
      COMMAND ${Python_EXECUTABLE} ${CMAKE_MODULE_PATH}/cppcheck-clean-compile-commands.py
              ${CMAKE_BINARY_DIR}/compile_commands.json --outfile ${CMAKE_BINARY_DIR}/compile_commands_cppcheck.json
      COMMAND ${CPPCHECK_EXECUTABLE} ${_cppcheck_args} ${_cppcheck_xml_args}
      WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
      COMMENT "Running cppcheck"
    )
    set_target_properties(cppcheck PROPERTIES EXCLUDE_FROM_ALL TRUE)
  endif()
endif(CPPCHECK_EXECUTABLE)
