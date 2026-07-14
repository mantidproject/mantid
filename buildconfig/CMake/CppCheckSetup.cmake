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

  # Cppcheck bundled in conda/pixi environments is compiled with a hardcoded DATADIR that typically does not match the
  # actual installation prefix, so --library=qt may fail to load qt.cfg. Resolve the full path to qt.cfg from the binary
  # location and pass it directly to --library so cppcheck can find it regardless of DATADIR.
  get_filename_component(_cppcheck_bin_dir "${CPPCHECK_EXECUTABLE}" DIRECTORY)
  set(_cppcheck_qt_cfg "${_cppcheck_bin_dir}/../share/Cppcheck/cfg/qt.cfg")
  get_filename_component(_cppcheck_qt_cfg "${_cppcheck_qt_cfg}" ABSOLUTE)
  if(EXISTS "${_cppcheck_qt_cfg}")
    set(_cppcheck_qt_library "${_cppcheck_qt_cfg}")
  else()
    set(_cppcheck_qt_library "qt")
  endif()

  # Set up the standard arguments --inline-suppr appears to be ignored if --suppressions-list is specified. Cppcheck >=
  # 2.10 requires all "file" entries within a compile_commands.json to exist whereas previously only a warning was
  # emitted if an entry didn't exist. Files such as qrc_*, moc_* files from Qt are listed but don't exist as they are
  # made by build rules. We filter the raw cmake-generated compile_commands.json below before executing cppcheck.
  set(CPPCHECK_ARGS
      --enable=all
      # 2.12 has missingInclude switched on by default if running with multiple cores but it doesn't appear to work
      # correctly. It was flagging many stl and other external header files as missing.
      --disable=missingInclude
      # Loading qt.cfg via full path suppresses Qt macro warnings (slots, signals, Q_OBJECT etc.) even when cppcheck is
      # installed via conda/pixi with a non-standard DATADIR. Falls back to --library=qt if the file is not found.
      --library=${_cppcheck_qt_library}
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

  # Arguments shared by the text ('cppcheck'), xml ('cppcheck-xml') and SARIF ('cppcheck-sarif') targets.
  set(_cppcheck_args "${CPPCHECK_ARGS}")
  if(CPPCHECK_NUM_THREADS GREATER 0)
    list(APPEND _cppcheck_args -j ${CPPCHECK_NUM_THREADS})
  endif(CPPCHECK_NUM_THREADS GREATER 0)

  # The 'cppcheck' target (run in CI) prints gcc-style diagnostics to stderr so they can be picked up by a problem
  # matcher. '_cppcheck_source_dirs' is empty in --project mode but kept for parity.
  set(_cppcheck_text_args ${_cppcheck_args} ${CPPCHECK_TEMPLATE_ARG} "${_cppcheck_source_dirs}")

  # The 'cppcheck-xml' target writes an xml report to cppcheck.xml (cppcheck emits the report on stderr, hence the
  # redirect). It is used for the HTML report and can also be used to regenerate the suppressions input when upgrading
  # cppcheck.
  set(_cppcheck_xml_args ${_cppcheck_args} --xml --xml-version=2 "${_cppcheck_source_dirs}")

  # The 'cppcheck-sarif' target writes a SARIF report to cppcheck.sarif. This can be consumed by GitHub code scanning
  # and by tools/Cppcheck/generate_cppcheck_suppressions_list.py when upgrading cppcheck.
  set(_cppcheck_sarif_args ${_cppcheck_args} --output-format=sarif --output-file="${CMAKE_BINARY_DIR}/cppcheck.sarif"
                           "${_cppcheck_source_dirs}"
  )

  if(NOT WIN32)
    message(STATUS "cppcheck configured to run")
    message(STATUS "remove the project argument to supply files to check")
    list(JOIN _cppcheck_text_args " " _cppcheck_args_for_printing)
    message(STATUS "${CPPCHECK_EXECUTABLE} ${_cppcheck_args_for_printing}")
  endif()

  # generate the text target (used in CI)
  if(NOT TARGET cppcheck)
    add_custom_target(
      cppcheck
      COMMAND ${Python_EXECUTABLE} ${CMAKE_MODULE_PATH}/cppcheck-clean-compile-commands.py
              ${CMAKE_BINARY_DIR}/compile_commands.json --outfile ${CMAKE_BINARY_DIR}/compile_commands_cppcheck.json
      COMMAND ${CPPCHECK_EXECUTABLE} ${_cppcheck_text_args}
      WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
      COMMENT "Running cppcheck"
    )
    set_target_properties(cppcheck PROPERTIES EXCLUDE_FROM_ALL TRUE)
  endif()

  # generate the xml-report target (used for the HTML report and to refresh the suppressions list)
  if(NOT TARGET cppcheck-xml)
    add_custom_target(
      cppcheck-xml
      COMMAND ${Python_EXECUTABLE} ${CMAKE_MODULE_PATH}/cppcheck-clean-compile-commands.py
              ${CMAKE_BINARY_DIR}/compile_commands.json --outfile ${CMAKE_BINARY_DIR}/compile_commands_cppcheck.json
      COMMAND ${CPPCHECK_EXECUTABLE} ${_cppcheck_xml_args} 2> ${CMAKE_BINARY_DIR}/cppcheck.xml
      WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
      COMMENT "Running cppcheck (writing xml report to cppcheck.xml)"
    )
    set_target_properties(cppcheck-xml PROPERTIES EXCLUDE_FROM_ALL TRUE)
  endif()

  # generate the SARIF-report target (used for SARIF consumers and to refresh the suppressions list)
  if(NOT TARGET cppcheck-sarif)
    add_custom_target(
      cppcheck-sarif
      COMMAND ${Python_EXECUTABLE} ${CMAKE_MODULE_PATH}/cppcheck-clean-compile-commands.py
              ${CMAKE_BINARY_DIR}/compile_commands.json --outfile ${CMAKE_BINARY_DIR}/compile_commands_cppcheck.json
      COMMAND ${CPPCHECK_EXECUTABLE} ${_cppcheck_sarif_args}
      WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
      COMMENT "Running cppcheck (writing SARIF report to cppcheck.sarif)"
    )
    set_target_properties(cppcheck-sarif PROPERTIES EXCLUDE_FROM_ALL TRUE)
  endif()
endif(CPPCHECK_EXECUTABLE)
