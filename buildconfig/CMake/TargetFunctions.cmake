function (mtd_install_targets)
  set (options)
  set (oneValueArgs TARGETS)
  set (multiValueArgs INSTALL_DIRS)
  cmake_parse_arguments (PARSED "${options}" "${oneValueArgs}"
                         "${multiValueArgs}" ${ARGN})

    if (NOT PARSED_INSTALL_DIRS)
        message(FATAL_ERROR "Empty argument INSTALL_DIRS")
        return()
    endif()
    if (NOT PARSED_TARGETS)
        message(FATAL_ERROR "Empty argument TARGETS")
        return()
    endif()

    list(REMOVE_DUPLICATES PARSED_INSTALL_DIRS)
    set ( _install_dirs ${PARSED_INSTALL_DIRS} )

    foreach( _dir ${_install_dirs})
        install ( TARGETS ${PARSED_TARGETS} ${SYSTEM_PACKAGE_TARGET} DESTINATION ${_dir} )
    endforeach ()
endfunction()

function (mtd_install_files)
  set (options)
  set (oneValueArgs RENAME)
  set (multiValueArgs FILES INSTALL_DIRS)
  cmake_parse_arguments (PARSED "${options}" "${oneValueArgs}"
                         "${multiValueArgs}" ${ARGN})

    if (NOT PARSED_INSTALL_DIRS)
        message(FATAL_ERROR "Empty argument INSTALL_DIRS")
        return()
    endif()
    if (NOT PARSED_FILES)
        message(FATAL_ERROR "Empty argument FILES")
        return()
    endif()
     list(REMOVE_DUPLICATES PARSED_INSTALL_DIRS)

     foreach( _dir ${PARSED_INSTALL_DIRS})
         install ( FILES ${PARSED_FILES} DESTINATION ${_dir} RENAME ${PARSED_RENAME} )
     endforeach ()
endfunction()

