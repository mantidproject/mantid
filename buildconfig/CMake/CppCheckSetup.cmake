find_package ( Cppcheck )

if ( CPPCHECK_EXECUTABLE )

  # We must export the compile commands for cppcheck to be able to check
  # everything correctly
  set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
  configure_file(${CMAKE_SOURCE_DIR}/buildconfig/CMake/CppCheck_Suppressions.txt.in ${CMAKE_BINARY_DIR}/CppCheck_Suppressions.txt)

  # setup the standard arguments
  # --inline-suppr appears to be ignored if --suppresions-list is specified
  set ( CPPCHECK_ARGS --enable=all --inline-suppr --max-configs=120
  --suppressions-list=${CMAKE_BINARY_DIR}/CppCheck_Suppressions.txt
  --project=${CMAKE_BINARY_DIR}/compile_commands.json
  # Force cppcheck to check when we use project-wide macros
  -DDLLExport=
  -DMANTID_ALGORITHMS_DLL=
  # Undefine problematic macros:
  #   Causes errors such as there was an internal error: bad macro syntax"
  -UQT_TESTCASE_BUILDDIR
  )

  set (_cppcheck_args "${CPPCHECK_ARGS}")
    list ( APPEND _cppcheck_args ${CPPCHECK_TEMPLATE_ARG} )
    if ( CPPCHECK_NUM_THREADS GREATER 0)
        list ( APPEND _cppcheck_args -j ${CPPCHECK_NUM_THREADS} )
  endif ( CPPCHECK_NUM_THREADS GREATER 0)

  # put the finishing bits on the final command call
  set (_cppcheck_xml_args)
  if (CPPCHECK_GENERATE_XML)
    list( APPEND _cppcheck_xml_args  --xml --xml-version=2 ${_cppcheck_source_dirs} 2> ${CMAKE_BINARY_DIR}/cppcheck.xml )
  else (CPPCHECK_GENERATE_XML)
    list( APPEND _cppcheck_xml_args  ${_cppcheck_source_dirs} )
  endif (CPPCHECK_GENERATE_XML)



  # generate the target
  if (NOT TARGET cppcheck)
    add_custom_target ( cppcheck
                        COMMAND ${CPPCHECK_EXECUTABLE} ${_cppcheck_args} ${_cppcheck_xml_args}
                        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                        COMMENT "Running cppcheck"
                      )
    set_target_properties(cppcheck PROPERTIES EXCLUDE_FROM_ALL TRUE)
  endif()
endif ( CPPCHECK_EXECUTABLE )
