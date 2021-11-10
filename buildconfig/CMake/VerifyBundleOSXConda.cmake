# Based on the CMake BundleUtilities file Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

# In our version we override gp_resolved_file_type_override so that libraries found on the rpath in the Frameworks
# folder are marked as embedded Usage: .. code-block:: cmake

# verify_app(<app>)

# Verifies that an application ``<app>`` appears valid based on running analysis tools on it.  Calls
# :command:`message(FATAL_ERROR)` if the application is not verified.

# The base implementation of verify_app doesn't work as we haven't renamed the binaries such that @rpath references
# haven't been replaced with their relative bundle path. Instead we rely on the r_path varaible encoded into the binary
# i.e its explicitly otool -l (....).dylib to return: @loader_path/../Frameworks/libtbb.dylib or
# @executable_path/../Frameworks/libtbb.dylib

# Instead it returns @rpath/../Frameworks/libtbb.dylib With rpath resolved with the binary LC_RPATH variable: cmd
# LC_RPATH cmdsize 40 path @loader_path/../Frameworks (offset 12)

# The BundleUtilities fixup_bundle function would typically replace all these @rpath with @executable_path. This however
# does not work for an application which has executables outside of the MacOS folder and non-flat directory structures.
# In our new pacakge the Python executable will be in the Resources/mantid_python/bin folder. Hence,
# @executable_path./../Frameworks would resolve to Resources/mantid_python/Frameworks - which is clearly wrong.

# A quote from a CMake Dev (https://cmake.org/pipermail/cmake/2011-February/042428.html) "If your bundle consists of
# executables located in more than one path depth..... BundleUtilities is not going to be sufficient, and you will have
# to use it simply as a guide for your own custom solution."

# This is beacuse it renames every @rpath reference to @executable_path assuming that you can always move one directory
# back from the executable and land in the Contents folder.

# cmake-format: off

# Do not include this module at configure time!
if(DEFINED CMAKE_GENERATOR)
  cmake_policy(GET CMP0080 _BundleUtilities_CMP0080)
  if(_BundleUtilities_CMP0080 STREQUAL "NEW")
    message(FATAL_ERROR "BundleUtilities cannot be included at configure time!")
  elseif(NOT _BundleUtilities_CMP0080 STREQUAL "OLD" AND NOT _CMP0080_SUPPRESS_WARNING)
    _warn_cmp0080()
  endif()
endif()

cmake_policy(PUSH)
cmake_policy(SET CMP0057 NEW) # if IN_LIST

include("GetPrerequisites")

# Override gp_resolved_file_type_override so that libraries found on the rpath in the Frameworks folder are marked as
# embedded
function(gp_resolved_file_type_override resolved_file type_var)

  get_filename_component(resolved_dir "${resolved_file}" ABSOLUTE)
  get_filename_component(resolved_parent_dir "${resolved_file}/../../" ABSOLUTE)
  # We need to figure out if the resolved file is in the bundle If we are in a frameworks folder we should be able to
  # move back one folder and find a MacOS folder - in which should be our MantidWorkbench or MantidWorkbenchUnstable
  # executable
  if(EXISTS "${resolved_parent_dir}/MacOS/MantidWorkbench" OR EXISTS
                                                              "${resolved_parent_dir}/MacOS/MantidWorkbenchUnstable"
  )
    set(${type_var}
        embedded
        PARENT_SCOPE
    )
  endif()

endfunction()

# Returns the main_executable of the bundle. The result will be the full path name of the bundle's main executable file
# or an ``error:`` prefixed string if it could not be determined.
function(get_bundle_main_executable bundle result_var)
  set(result "error: '${bundle}/Contents/Info.plist' file does not exist")

  if(EXISTS "${bundle}/Contents/Info.plist")
    set(result "error: no CFBundleExecutable in '${bundle}/Contents/Info.plist' file")
    set(line_is_main_executable 0)
    set(bundle_executable "")

    # Read Info.plist as a list of lines:
    #
    set(eol_char "E")
    file(READ "${bundle}/Contents/Info.plist" info_plist)
    string(REPLACE ";" "\\;" info_plist "${info_plist}")
    string(REPLACE "\n" "${eol_char};" info_plist "${info_plist}")
    string(REPLACE "\r" "${eol_char};" info_plist "${info_plist}")

    # Scan the lines for "<key>CFBundleExecutable</key>" - the line after that is the name of the main executable.
    #
    foreach(line ${info_plist})
      if(line_is_main_executable)
        string(REGEX REPLACE "^.*<string>(.*)</string>.*$" "\\1" bundle_executable "${line}")
        break()
      endif()

      if(line MATCHES "<key>CFBundleExecutable</key>")
        set(line_is_main_executable 1)
      endif()
    endforeach()

    if(NOT bundle_executable STREQUAL "")
      if(EXISTS "${bundle}/Contents/MacOS/${bundle_executable}")
        set(result "${bundle}/Contents/MacOS/${bundle_executable}")
      else()

        # Ultimate goal: If not in "Contents/MacOS" then scan the bundle for matching files. If there is only one
        # executable file that matches, then use it, otherwise it's an error...
        #
        # file(GLOB_RECURSE file_list "${bundle}/${bundle_executable}")

        # But for now, pragmatically, it's an error. Expect the main executable for the bundle to be in Contents/MacOS,
        # it's an error if it's not:
        #
        set(result "error: '${bundle}/Contents/MacOS/${bundle_executable}' does not exist")
      endif()
    endif()
  else()
    #
    # More inclusive technique... (This one would work on Windows and Linux too, if a developer followed the typical Mac
    # bundle naming convention...)
    #
    # If there is no Info.plist file, try to find an executable with the same base name as the .app directory:
    #
  endif()

  set(${result_var}
      "${result}"
      PARENT_SCOPE
  )
endfunction()

# Returns the main_libraries of the bundle. These are the libraries in the MacOS folder The result will be the full path
# name of the bundle's main executable file or an ``error:`` prefixed string if it could not be determined. Scans
# ``<bundle>`` bundle recursively for all libraries files and accumulates them into a variable.
function(get_bundle_all_libraries bundle library_var)
  set(libs "")
  if(UNIX)
    find_program(find_cmd "find")
    mark_as_advanced(find_cmd)
  endif()

  # find command is much quicker than checking every file one by one on Unix which can take long time for large bundles,
  # and since anyway we expect executable to have execute flag set we can narrow the list much quicker.
  if(find_cmd)
    execute_process(COMMAND "${find_cmd}" "${bundle}/Contents/MacOS"
      -type f \( -perm -0100 -o -perm -0010 -o -perm -0001 \)
      OUTPUT_VARIABLE file_list
      OUTPUT_STRIP_TRAILING_WHITESPACE
      )
    string(REPLACE "\n" ";" file_list "${file_list}")
  else()
    file(GLOB_RECURSE file_list "${bundle}/Contents/MacOS/*")
  endif()

  foreach(f ${file_list})
    get_filename_component(file_ext ${f} EXT)
    if(file_ext STREQUAL ".dylib")
      set(libs ${libs} "${f}")
    endif()
  endforeach()

  set(${library_var}
      "${libs}"
      PARENT_SCOPE
  )
endfunction()

function(get_item_rpaths item rpaths_var)
  if(APPLE)
    find_program(otool_cmd "otool")
    mark_as_advanced(otool_cmd)
  endif()

  if(otool_cmd)
    execute_process(
      COMMAND "${otool_cmd}" -l "${item}"
      OUTPUT_VARIABLE load_cmds_ov
      RESULT_VARIABLE otool_rv
      ERROR_VARIABLE otool_ev
    )
    if(NOT otool_rv STREQUAL "0")
      message(FATAL_ERROR "otool -l failed: ${otool_rv}\n${otool_ev}")
    endif()
    string(REGEX REPLACE "[^\n]+cmd LC_RPATH\n[^\n]+\n[^\n]+path ([^\n]+) \\(offset[^\n]+\n" "rpath \\1\n" load_cmds_ov
                         "${load_cmds_ov}"
    )
    string(REGEX MATCHALL "rpath [^\n]+" load_cmds_ov "${load_cmds_ov}")
    string(REGEX REPLACE "rpath " "" load_cmds_ov "${load_cmds_ov}")
    if(load_cmds_ov)
      foreach(rpath ${load_cmds_ov})
        gp_append_unique(${rpaths_var} "${rpath}")
      endforeach()
    endif()
  endif()

  if(UNIX AND NOT APPLE)
    file(
      READ_ELF
      ${item}
      RPATH
      rpath_var
      RUNPATH
      runpath_var
      CAPTURE_ERROR
      error_var
    )
    get_filename_component(item_dir ${item} DIRECTORY)
    foreach(rpath ${rpath_var} ${runpath_var})
      # Substitute $ORIGIN with the exepath and add to the found rpaths
      string(REPLACE "$ORIGIN" "${item_dir}" rpath "${rpath}")
      gp_append_unique(${rpaths_var} "${rpath}")
    endforeach()
  endif()

  set(${rpaths_var}
      ${${rpaths_var}}
      PARENT_SCOPE
  )
endfunction()

function(get_bundle_and_executable app bundle_var executable_var valid_var)
  set(valid 0)

  if(EXISTS "${app}")
    # Is it a directory ending in .app?
    if(IS_DIRECTORY "${app}")
      if(app MATCHES "\\.app$")
        get_bundle_main_executable("${app}" executable)
        if(EXISTS "${app}" AND EXISTS "${executable}")
          set(${bundle_var}
              "${app}"
              PARENT_SCOPE
          )
          set(${executable_var}
              "${executable}"
              PARENT_SCOPE
          )
          set(valid 1)
        else()
          message(STATUS "warning: *NOT* handled - .app directory case...")
        endif()
      else()
        message(STATUS "warning: *NOT* handled - directory but not .app case...")
      endif()
    else()
      # Is it an executable file?
      is_file_executable("${app}" is_executable)
      if(is_executable)
        get_dotapp_dir("${app}" dotapp_dir)
        if(EXISTS "${dotapp_dir}")
          set(${bundle_var}
              "${dotapp_dir}"
              PARENT_SCOPE
          )
          set(${executable_var}
              "${app}"
              PARENT_SCOPE
          )
          set(valid 1)
          # message(STATUS "info: handled executable file in .app dir case...")
        else()
          get_filename_component(app_dir "${app}" PATH)
          set(${bundle_var}
              "${app_dir}"
              PARENT_SCOPE
          )
          set(${executable_var}
              "${app}"
              PARENT_SCOPE
          )
          set(valid 1)
          # message(STATUS "info: handled executable file in any dir case...")
        endif()
      else()
        message(STATUS "warning: *NOT* handled - not .app dir, not executable file...")
      endif()
    endif()
  else()
    message(STATUS "warning: *NOT* handled - directory/file does not exist...")
  endif()

  if(NOT valid)
    set(${bundle_var}
        "error: not a bundle"
        PARENT_SCOPE
    )
    set(${executable_var}
        "error: not a bundle"
        PARENT_SCOPE
    )
  endif()

  set(${valid_var}
      ${valid}
      PARENT_SCOPE
  )
endfunction()

# Scans ``<bundle>`` bundle recursively for all ``<exes_var>`` executable files and accumulates them into a variable.
function(get_bundle_all_executables bundle exes_var)
  set(exes "")

  if(UNIX)
    find_program(find_cmd "find")
    mark_as_advanced(find_cmd)
  endif()

  # find command is much quicker than checking every file one by one on Unix which can take long time for large bundles,
  # and since anyway we expect executable to have execute flag set we can narrow the list much quicker.
  if(find_cmd)
    execute_process(COMMAND "${find_cmd}" "${bundle}"
      -type f \( -perm -0100 -o -perm -0010 -o -perm -0001 \)
      OUTPUT_VARIABLE file_list
      OUTPUT_STRIP_TRAILING_WHITESPACE
      )
    string(REPLACE "\n" ";" file_list "${file_list}")
  else()
    file(GLOB_RECURSE file_list "${bundle}/*")
  endif()

  foreach(f ${file_list})
    is_file_executable("${f}" is_executable)
    if(is_executable)
      set(exes ${exes} "${f}")
    endif()
  endforeach()

  set(${exes_var}
      "${exes}"
      PARENT_SCOPE
  )
endfunction()
function(get_item_rpaths item rpaths_var)
  if(APPLE)
    find_program(otool_cmd "otool")
    mark_as_advanced(otool_cmd)
  endif()

  if(otool_cmd)
    execute_process(
      COMMAND "${otool_cmd}" -l "${item}"
      OUTPUT_VARIABLE load_cmds_ov
      RESULT_VARIABLE otool_rv
      ERROR_VARIABLE otool_ev
    )
    if(NOT otool_rv STREQUAL "0")
      message(FATAL_ERROR "otool -l failed: ${otool_rv}\n${otool_ev}")
    endif()
    string(REGEX REPLACE "[^\n]+cmd LC_RPATH\n[^\n]+\n[^\n]+path ([^\n]+) \\(offset[^\n]+\n" "rpath \\1\n" load_cmds_ov
                         "${load_cmds_ov}"
    )
    string(REGEX MATCHALL "rpath [^\n]+" load_cmds_ov "${load_cmds_ov}")
    string(REGEX REPLACE "rpath " "" load_cmds_ov "${load_cmds_ov}")
    if(load_cmds_ov)
      foreach(rpath ${load_cmds_ov})
        gp_append_unique(${rpaths_var} "${rpath}")
      endforeach()
    endif()
  endif()

  if(UNIX AND NOT APPLE)
    file(
      READ_ELF
      ${item}
      RPATH
      rpath_var
      RUNPATH
      runpath_var
      CAPTURE_ERROR
      error_var
    )
    get_filename_component(item_dir ${item} DIRECTORY)
    foreach(rpath ${rpath_var} ${runpath_var})
      # Substitute $ORIGIN with the exepath and add to the found rpaths
      string(REPLACE "$ORIGIN" "${item_dir}" rpath "${rpath}")
      gp_append_unique(${rpaths_var} "${rpath}")
    endforeach()
  endif()

  set(${rpaths_var}
      ${${rpaths_var}}
      PARENT_SCOPE
  )
endfunction()

# Verifies that the sum of all prerequisites of all files inside the bundle are contained within the bundle or are
# ``system`` libraries, presumed to exist everywhere.
function(verify_bundle_prerequisites bundle result_var info_var)
  set(result 1)
  set(info "")
  set(count 0)

  set(options)
  set(oneValueArgs)
  set(multiValueArgs IGNORE_ITEM)
  cmake_parse_arguments(CFG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  get_bundle_main_executable("${bundle}" main_bundle_exe)

  get_bundle_all_executables("${bundle}" file_list)

  get_bundle_all_libraries("${app}" framework_libraries)

  # Scan over the exes
  foreach(f ${file_list})
    get_filename_component(exepath "${f}" PATH)
    math(EXPR count "${count} + 1")

    message(STATUS "executable file ${count}: ${f}")

    set(prereqs "")
    get_filename_component(prereq_filename ${f} NAME)

    if(NOT prereq_filename IN_LIST CFG_IGNORE_ITEM)
      get_item_rpaths(${f} _main_exe_rpaths)
      string(REPLACE "@executable_path" "${exepath}" dirs "${_main_exe_rpaths}")

      get_prerequisites(
        "${f}"
        prereqs
        1
        1
        "${exepath}"
        "${dirs}"
        "${_main_exe_rpaths}"
      )

      # On the Mac, "embedded" and "system" prerequisites are fine... anything else means the bundle's prerequisites are
      # not verified (i.e., the bundle is not really "standalone")
      #
      # On Windows (and others? Linux/Unix/...?) "local" and "system" prereqs are fine...
      #

      set(external_prereqs "")

      # We found all the preq - now make sure they were shipped internally / on the system

      foreach(p ${prereqs})
        set(p_type "")
        gp_resolved_file_type("${f}" "${p}" "${exepath}" "${dirs}" p_type "${dirs}")

        if(APPLE)
          if(NOT p_type STREQUAL "embedded" AND NOT p_type STREQUAL "system")
            set(external_prereqs ${external_prereqs} "${p}")
          endif()
        else()
          if(NOT p_type STREQUAL "local" AND NOT p_type STREQUAL "system")
            set(external_prereqs ${external_prereqs} "${p}")
          endif()
        endif()
      endforeach()

      if(external_prereqs)
        # Found non-system/somehow-unacceptable prerequisites:
        set(result 0)
        set(info ${info} "external prerequisites found:\nf='${f}'\nexternal_prereqs='${external_prereqs}'\n")
      endif()
    else()
      message(STATUS "Ignoring file: ${prereq_filename}")
    endif()
  endforeach()
  set(count 0)

  # Scan over libs in MacOS folder to make sure they are self contained
  foreach(f ${framework_libraries})
    get_filename_component(exepath "${f}" PATH)
    math(EXPR count "${count} + 1")

    message(STATUS "Library file ${count}: ${f}")

    set(prereqs "")
    get_filename_component(prereq_filename ${f} NAME)

    if(NOT prereq_filename IN_LIST CFG_IGNORE_ITEM)
      get_item_rpaths(${f} _main_exe_rpaths)
      string(REPLACE "@executable_path" "${exepath}" dirs "${_main_exe_rpaths}")

      get_prerequisites(
        "${f}"
        prereqs
        1
        1
        "${exepath}"
        "${dirs}"
        "${_main_exe_rpaths}"
      )

      # On the Mac, "embedded" and "system" prerequisites are fine... anything else means the bundle's prerequisites are
      # not verified (i.e., the bundle is not really "standalone")
      #
      # On Windows (and others? Linux/Unix/...?) "local" and "system" prereqs are fine...
      #

      set(external_prereqs "")

      # We found all the preq - now make sure they were shipped internally / on the system

      foreach(p ${prereqs})
        set(p_type "")
        gp_resolved_file_type("${f}" "${p}" "${exepath}" "${dirs}" p_type "${dirs}")

        if(APPLE)
          if(NOT p_type STREQUAL "embedded" AND NOT p_type STREQUAL "system")
            set(external_prereqs ${external_prereqs} "${p}")
          endif()
        else()
          if(NOT p_type STREQUAL "local" AND NOT p_type STREQUAL "system")
            set(external_prereqs ${external_prereqs} "${p}")
          endif()
        endif()
      endforeach()

      if(external_prereqs)
        # Found non-system/somehow-unacceptable prerequisites:
        set(result 0)
        set(info ${info} "external prerequisites found:\nf='${f}'\nexternal_prereqs='${external_prereqs}'\n")
      endif()
    else()
      message(STATUS "Ignoring file: ${prereq_filename}")
    endif()
  endforeach()

  if(result)
    set(info "Verified ${count} executable files in '${bundle}'")
  endif()

  set(${result_var}
      "${result}"
      PARENT_SCOPE
  )
  set(${info_var}
      "${info}"
      PARENT_SCOPE
  )
endfunction()

# Verifies that an application ``<app>`` appears valid based on running analysis tools on it.  Calls
# :command:`message(FATAL_ERROR)` if the application is not verified.
function(verify_app app)
  set(verified 0)
  set(info "")

  set(options)
  set(oneValueArgs)
  set(multiValueArgs IGNORE_ITEM)
  cmake_parse_arguments(CFG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  get_bundle_and_executable("${app}" bundle executable valid)

  message(STATUS "===========================================================================")
  message(STATUS "Analyzing app='${app}'")
  message(STATUS "bundle='${bundle}'")
  message(STATUS "executable='${executable}'")
  message(STATUS "valid='${valid}'")

  # Verify that the bundle does not have any "external" prerequisites:
  #
  verify_bundle_prerequisites("${bundle}" verified info IGNORE_ITEM "${CFG_IGNORE_ITEM}")
  message(STATUS "verified='${verified}'")
  message(STATUS "info='${info}'")
  message(STATUS "")

  if(NOT verified)
    message(FATAL_ERROR "error: verify_app failed")
  endif()
endfunction()

function(verify_app app)
  set(verified 0)
  set(info "")

  set(options)
  set(oneValueArgs)
  set(multiValueArgs IGNORE_ITEM)
  cmake_parse_arguments(CFG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  get_bundle_and_executable("${app}" bundle executable valid)

  message(STATUS "===========================================================================")
  message(STATUS "Analyzing app='${app}'")
  message(STATUS "bundle='${bundle}'")
  message(STATUS "executable='${executable}'")
  message(STATUS "valid='${valid}'")

  # Verify that the bundle does not have any "external" prerequisites:
  #
  verify_bundle_prerequisites("${bundle}" verified info IGNORE_ITEM "${CFG_IGNORE_ITEM}")
  message(STATUS "verified='${verified}'")
  message(STATUS "info='${info}'")
  message(STATUS "")

  if(NOT verified)
    message(FATAL_ERROR "error: verify_app failed")
  endif()
endfunction()

cmake_policy(POP)

# cmake-format: on
