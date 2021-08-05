# Install a target into multiple directories
# This respects ENABLE_WORKBENCH flags for macOS
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
    _sanitize_install_dirs(_install_dirs ${PARSED_INSTALL_DIRS})
    foreach( _dir ${_install_dirs})
        install ( TARGETS ${PARSED_TARGETS} ${SYSTEM_PACKAGE_TARGET} DESTINATION ${_dir} )
    endforeach ()
endfunction()


# Install a framework library (used primarily for a conda install)
function (mtd_install_framework_lib)
set (options INSTALL_EXPORT_FILE PLUGIN_LIB)
set (oneValueArgs TARGETS EXPORT_NAME)
set (multiValueArgs INSTALL_DIRS)
cmake_parse_arguments (PARSED "${options}" "${oneValueArgs}"
"${multiValueArgs}" ${ARGN})
# if its a plugin we don't need to headers or .lib file
# we also don't need to export the cmake targets
if (PARSED_PLUGIN_LIB)
install(TARGETS ${PARSED_TARGETS}
RUNTIME DESTINATION plugins
LIBRARY DESTINATION plugins
)
else()
install(
    DIRECTORY inc/
    DESTINATION include/Mantid
    COMPONENT Devel
    PATTERN ".in" EXCLUDE)
if(PARSED_INSTALL_EXPORT_FILE)
    install(
        FILES ${CMAKE_CURRENT_BINARY_DIR}/Mantid${PARSED_TARGETS}/DllConfig.h
        DESTINATION include/Mantid/Mantid${PARSED_TARGETS}
        COMPONENT Devel)
endif()
install ( TARGETS ${PARSED_TARGETS}
EXPORT ${PARSED_EXPORT_NAME}
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin)

install(EXPORT ${PARSED_EXPORT_NAME}
    FILE ${PARSED_EXPORT_NAME}.cmake
    NAMESPACE Mantid::
    COMPONENT Devel
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/MantidFramework
)
endif()
endfunction()


# Install files into multiple directories
# This respects ENABLE_WORKBENCH flags for macOS
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

     _sanitize_install_dirs(_install_dirs ${PARSED_INSTALL_DIRS})
     foreach( _dir ${_install_dirs})
         # install (FILES ) only overwrites file if timestamp is different. Touch files here to always overwrite
         # Wrap call to execute_process in install (CODE ) so it runs at package time and not build time
         install ( CODE "execute_process(COMMAND \"${CMAKE_COMMAND}\" -E touch \"${PARSED_FILES}\")")
         install ( FILES ${PARSED_FILES} DESTINATION ${_dir} RENAME ${PARSED_RENAME} )
     endforeach ()
endfunction()

# Install directories into multiple directories
# This respects ENABLE_WORKBENCH flags for macOS
function (mtd_install_dirs)
    set (options)
    set (oneValueArgs DIRECTORY EXCLUDE)
    set (multiValueArgs INSTALL_DIRS PATTERN)
    cmake_parse_arguments (PARSED "${options}" "${oneValueArgs}"
                         "${multiValueArgs}" ${ARGN})

    if (NOT PARSED_INSTALL_DIRS)
        message(FATAL_ERROR "Empty argument INSTALL_DIRS")
        return()
    endif()
    if (NOT PARSED_DIRECTORY)
        message(FATAL_ERROR "Empty argument FILES")
        return()
    endif()

    _sanitize_install_dirs(_install_dirs ${PARSED_INSTALL_DIRS})
    foreach( _dir ${_install_dirs})
        install(DIRECTORY ${PARSED_DIRECTORY}
                DESTINATION ${_dir}
                PATTERN ${PARSED_EXCLUDE} EXCLUDE)
    endforeach()
endfunction()

# Private function to sanitize the list of install dirs
# against duplicates and taking into account ENABLE_WORKBENCH
function (_sanitize_install_dirs output_variable)
    set(potential_dirs ${ARGN})
    # Linux/windows packages share install layouts for many things
    # This ensures only a single unique install directory is present
    list(REMOVE_DUPLICATES potential_dirs)
    if(NOT ENABLE_WORKBENCH)
      set(${output_variable} ${potential_dirs} PARENT_SCOPE)
      return()
    endif()
    # Mac has 2 separate bundles so we need to check the status of
    # ENABLE_MANTIDWORKBENCH
    if(APPLE)
      set(all_dirs ${potential_dirs})
      set(potential_dirs)
      foreach( _dir ${all_dirs})
        if(ENABLE_WORKBENCH AND ${_dir} MATCHES "${WORKBENCH_BUNDLE}.*")
	  list(APPEND potential_dirs ${_dir})
        endif()
      endforeach()
    endif()
    set(${output_variable} ${potential_dirs} PARENT_SCOPE)
endfunction()
