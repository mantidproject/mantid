# Utility functions to add libraries/executables parametrized by the required version of Qt
include(TargetFunctions)

# name: mtd_add_qt_library brief: create a library target for linked against Qt The global ENABLE_WORKBENCH option
# controls if a Qt5 target is created. To limit the Qt version for a specific library use QT_VERSION, e.g.
#
# mtd_add_qt_library ( QT_VERSION 5 ... ... )
#
# For all options see mtd_add_qt_target
function(mtd_add_qt_library)
  _qt_versions(_qt_vers ${ARGN})
  # Create targets
  foreach(_ver ${_qt_vers})
    if(_ver EQUAL 5 AND (ENABLE_WORKBENCH OR BUILD_MANTIDQT))
      mtd_add_qt_target(LIBRARY QT_VERSION ${_ver} ${ARGN})
    endif()
  endforeach()
endfunction()

# name: mtd_add_qt_executable brief: create a library target for linked against Qt The global ENABLE_WORKBENCH option
# controls if a Qt5 target is created. To limit the Qt version for a specific library use QT_VERSION, e.g.
#
# mtd_add_qt_executable ( QT_VERSION 5 ... ... ) For all options see mtd_add_qt_target
function(mtd_add_qt_executable)
  _qt_versions(_qt_vers ${ARGN})
  # Create targets
  foreach(_ver ${_qt_vers})
    if(_ver EQUAL 5 AND (ENABLE_WORKBENCH OR BUILD_MANTIDQT))
      mtd_add_qt_target(EXECUTABLE QT_VERSION ${_ver} ${ARGN})
    endif()
  endforeach()
endfunction()

# Target agnostic function to add either an executable or library linked to Qt:
#
# * option: LIBRARY If included define a library target
# * option: EXECUTABLE If included define an executable target
# * option: NO_SUFFIX If included, no suffix is added to the target name
# * option: EXCLUDE_FROM_ALL If included, the target is excluded from target ALL
# * keyword: TARGET_NAME The name of the target. The target will have -Qt{QT_VERSION} appended to it.
# * keyword: OUTPUT_NAME An optional filename for the library
# * keyword: QT_VERSION The major version of Qt to build against
# * keyword: SRC .cpp files to include in the target build
# * keyword: QT5_SRC .cpp files to include in a Qt5 build
# * keyword: MOC Header files that are to be parsed by moc
# * keyword: UI Qt designer ui files that are to be parsed by the UI compiler
# * keyword: NOMOC Additional headers that are not to be passed to moc
# * keyword: RES Any resource .qrc files
# * keyword: DEFS Compiler definitions to set for all targets. Also QTX_DEFS can be used to set per-version targets
# * keyword: OUTPUT_DIR_BASE Base directory the build output. The final product goes into a subdirectory based on the Qt
#   version
# * keyword: OUTPUT_SUBDIR Additional directory to added to the final output path
# * keyword: INCLUDE_DIRS A list of include directories to add to the target
# * keyword: SYSTEM_INCLUDE_DIRS A list of include directories to add to the target and marked as system headers
# * keyword: PRECOMPILED A name of the precompiled header
# * keyword: LINK_LIBS A list of additional libraries to link to the target that are not dependent on Qt
# * keyword: QT5_LINK_LIBS A list of additional Qt libraries to link to. QtWidgets is linked to by default
# * keyword: MTD_QT_LINK_LIBS A list of additional libraries to link to the target. It is assumed each was produced with
#   this function and will have the -Qt{QT_VERSION} suffix appended.
# * keyword: INSTALL_DIR A destination directory for the install command.
# * keyword: INSTALL_DIR_BASE Base directory the build output. The final product goes into a subdirectory based on the
#   Qt version.
# * keyword: OSX_INSTALL_RPATH Install path for osx version > 10.8
# * keyword: LINUX_INSTALL_RPATH Install path for CMAKE_SYSTEM_NAME == Linux
function(mtd_add_qt_target)
  set(options LIBRARY EXECUTABLE NO_SUFFIX EXCLUDE_FROM_ALL)
  set(oneValueArgs TARGET_NAME OUTPUT_NAME QT_VERSION OUTPUT_DIR_BASE OUTPUT_SUBDIR PRECOMPILED)
  set(multiValueArgs
      SRC
      UI
      MOC
      NOMOC
      RES
      DEFS
      QT5_DEFS
      INCLUDE_DIRS
      SYSTEM_INCLUDE_DIRS
      LINK_LIBS
      QT5_LINK_LIBS
      MTD_QT_LINK_LIBS
      OSX_INSTALL_RPATH
      LINUX_INSTALL_RPATH
      INSTALL_DIR
      INSTALL_DIR_BASE
  )
  cmake_parse_arguments(PARSED "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
  if(PARSED_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR "Unexpected arguments found: ${PARSED_UNPARSED_ARGUMENTS}")
  endif()

  if(${PARSED_LIBRARY} AND ${PARSED_EXECUTABLE})
    message(FATAL_ERROR "Both LIBRARY and EXECUTABLE options specified. Please choose only one.")
  endif()

  # uic needs to run against the correct version of Qt. Keep the generated files in a subdirectory of the binary
  # directory labelled by the version
  set(_binary_dir_on_entry ${CMAKE_CURRENT_BINARY_DIR})
  _append_qt_suffix(AS_DIR VERSION ${PARSED_QT_VERSION} OUTPUT_VARIABLE _ui_dir ${CMAKE_CURRENT_BINARY_DIR})
  set(CMAKE_CURRENT_BINARY_DIR ${_ui_dir})
  set(_all_defines ${PARSED_DEFS};${PARSED_QT${PARSED_QT_VERSION}_DEFS})
  if(PARSED_QT_VERSION EQUAL 5)
    qt5_wrap_ui(UI_HEADERS ${PARSED_UI})
    _internal_qt_wrap_cpp(5 MOC_GENERATED DEFS ${_all_defines} INFILES ${PARSED_MOC})
    set(ALL_SRC ${PARSED_SRC} ${PARSED_QT5_SRC} ${MOC_GENERATED})
    qt5_add_resources(RES_FILES ${PARSED_RES})
    set(_qt_link_libraries Qt5::Widgets ${PARSED_QT5_LINK_LIBS})
  else()
    message(FATAL_ERROR "Unknown Qt version. Please specify only the major version.")
  endif()

  if(UI_HEADERS)
    file(GLOB EXISTING_UI_HEADERS ${CMAKE_CURRENT_BINARY_DIR}/ui_*.h)
    foreach(_existing_hdr ${EXISTING_UI_HEADERS})
      if(NOT _existing_hdr IN_LIST UI_HEADERS)
        message("Deleting orphaned ui_*.h file: " ${_existing_hdr})
        file(REMOVE ${_existing_hdr})
      endif()
    endforeach()
  endif()

  set(CMAKE_CURRENT_BINARY_DIR ${_binary_dir_on_entry})

  if(PARSED_NO_SUFFIX)
    set(_target ${PARSED_TARGET_NAME})
    set(_output_name ${PARSED_OUTPUT_NAME})
  else()
    _append_qt_suffix(VERSION ${PARSED_QT_VERSION} OUTPUT_VARIABLE _target ${PARSED_TARGET_NAME})
    _append_qt_suffix(VERSION ${PARSED_QT_VERSION} OUTPUT_VARIABLE _output_name ${PARSED_OUTPUT_NAME})
  endif()
  _append_qt_suffix(VERSION ${PARSED_QT_VERSION} OUTPUT_VARIABLE _mtd_qt_libs ${PARSED_MTD_QT_LINK_LIBS})

  if(PARSED_EXCLUDE_FROM_ALL)
    set(_target_exclude_from_all "EXCLUDE_FROM_ALL")
  else()
    set(_target_exclude_from_all)
  endif()

  # Use a precompiled header where they are supported
  if(PARSED_PRECOMPILED)
    enable_precompiled_headers(${PARSED_PRECOMPILED} ALL_SRC)
  endif()

  if(PARSED_LIBRARY)
    add_library(
      ${_target} ${_target_exclude_from_all} ${ALL_SRC} ${UI_HEADERS} ${PARSED_MOC} ${PARSED_NOMOC} ${RES_FILES}
    )
  elseif(PARSED_EXECUTABLE)
    add_executable(
      ${_target} ${_target_exclude_from_all} ${ALL_SRC} ${UI_HEADERS} ${PARSED_NOMOC} ${PARSED_NOMOC} ${RES_FILES}
    )
  else()
    message(FATAL_ERROR "Unknown target type. Options=LIBRARY,EXECUTABLE")
  endif()

  # Target properties
  if(_output_name)
    set_target_properties(${_target} PROPERTIES OUTPUT_NAME ${_output_name})
  endif()
  if(PARSED_OUTPUT_DIR_BASE)
    set(_output_dir ${PARSED_OUTPUT_DIR_BASE}/qt${PARSED_QT_VERSION})
    if(PARSED_OUTPUT_SUBDIR)
      set(_output_dir ${_output_dir}/${PARSED_OUTPUT_SUBDIR})
    endif()
    set_target_properties(
      ${_target} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${_output_dir} RUNTIME_OUTPUT_DIRECTORY ${_output_dir}
    )
  endif()
  set_target_properties(${_target} PROPERTIES CXX_CLANG_TIDY "")
  _disable_suggest_override(${PARSED_QT_VERSION} ${_target})
  # Use public headers to populate the INTERFACE_INCLUDE_DIRECTORIES target property
  target_include_directories(${_target} PUBLIC ${_ui_dir} ${_other_ui_dirs} ${PARSED_INCLUDE_DIRS})
  if(PARSED_SYSTEM_INCLUDE_DIRS)
    target_include_directories(${_target} SYSTEM PUBLIC ${PARSED_SYSTEM_INCLUDE_DIRS})
  endif()

  target_link_libraries(${_target} PUBLIC ${_qt_link_libraries} ${PARSED_LINK_LIBS} ${_mtd_qt_libs})
  if(_all_defines)
    set_target_properties(${_target} PROPERTIES COMPILE_DEFINITIONS "${_all_defines}")
  endif()

  if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    if(PARSED_OSX_INSTALL_RPATH)
      set_target_properties(${_target} PROPERTIES INSTALL_RPATH "${PARSED_OSX_INSTALL_RPATH}")
    endif()
  elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    if(PARSED_LINUX_INSTALL_RPATH)
      set_target_properties(${_target} PROPERTIES INSTALL_RPATH "${PARSED_LINUX_INSTALL_RPATH}")
    endif()
  endif()

  if(PARSED_EXCLUDE_FROM_ALL)
    set_target_properties(${_target} PROPERTIES EXCLUDE_FROM_ALL TRUE)
  else()

    if(PARSED_INSTALL_DIR AND PARSED_INSTALL_DIR_BASE)
      message(FATAL "Cannot provide both INSTALL_DIR and PARSED_INSTALL_DIR_BASE options")
    endif()

    if(PARSED_INSTALL_DIR)
      list(REMOVE_DUPLICATES PARSED_INSTALL_DIR)
      set(_install_dirs ${PARSED_INSTALL_DIR})
    elseif(PARSED_INSTALL_DIR_BASE)
      list(REMOVE_DUPLICATES PARSED_INSTALL_DIR_BASE)

      set(_install_dirs)
      foreach(_dir ${PARSED_INSTALL_DIR_BASE})
        _append_qt_suffix(AS_DIR VERSION ${PARSED_QT_VERSION} OUTPUT_VARIABLE _dir ${PARSED_INSTALL_DIR_BASE})
        list(APPEND _install_dirs ${_dir})
      endforeach()
    else()
      set(_install_dirs "")
      message(FATAL_ERROR "Target: ${_target} is configured to build but has no install destination")
    endif()

    foreach(_dir ${_install_dirs})
      mtd_install_qt_library(${PARSED_QT_VERSION} ${_target} ${_dir})
    endforeach()
  endif()

  # Group into folder for VS
  set_target_properties(${_target} PROPERTIES FOLDER "Qt${PARSED_QT_VERSION}")
  # Target encompassing all Qt-based dependencies
  set(_alltarget "AllQt${PARSED_QT_VERSION}")
  if(TARGET ${_alltarget})
    add_dependencies(${_alltarget} ${_target})
  else()
    add_custom_target(${_alltarget} DEPENDS ${_target})
  endif()

endfunction()

# Create an install rule for a Qt target - qt_version: The version of Qt targeted. target: The name of the target -
# install_target_type: The type of target that should be installed. See
# https://cmake.org/cmake/help/latest/command/install.html?highlight=install - install_dir A relative directory to
# install_prefix
function(mtd_install_qt_library qt_version target install_dir)
  get_target_property(target_type ${target} TYPE)
  set(install_target_type "")
  if(WIN32 AND NOT target_type STREQUAL "MODULE_LIBRARY")
    # Install only DLLs on Windows
    set(install_target_type RUNTIME)
  endif()

  mtd_install_shared_library(TARGETS ${target} ${install_target_type} DESTINATION ${install_dir})
endfunction()

function(mtd_add_qt_tests)
  _qt_versions(_qt_vers ${ARGN})
  # Create test executables
  foreach(_ver ${_qt_vers})
    if(_ver EQUAL 5 AND (ENABLE_WORKBENCH OR BUILD_MANTIDQT))
      mtd_add_qt_test_executable(QT_VERSION ${_ver} ${ARGN})
    endif()
  endforeach()
endfunction()

# Create an executable target for running a set of unit tests linked to Qt keyword: TEST_NAME The name of the test
# suite. The version of Qt will be appended to it keyword: SRC The list of test headers containing the unit test code
# keyword: QT_VERSION The major version of Qt to build against keyword: INCLUDE_DIRS A list of include directories to
# add to the target keyword: TEST_HELPER_SRCS A list of test helper files to compile in with the target keyword:
# LINK_LIBS A list of additional libraries to link to the target that are not dependent on Qt keyword: QT5_LINK_LIBS A
# list of additional Qt libraries to link to. QtWidgets islinked to by default keyword: MTD_QT_LINK_LIBS A list of
# additional libraries to link to the target. It is assumed each was produced with this function and will have the
# Qt{QT_VERSION} suffix appended. keyword: PARENT_DEPENDENCIES Any targets listed here will have this new target as a
# dependency
function(mtd_add_qt_test_executable)
  set(options)
  set(oneValueArgs TARGET_NAME QT_VERSION)
  set(multiValueArgs
      SRC
      INCLUDE_DIRS
      TEST_HELPER_SRCS
      LINK_LIBS
      QT5_LINK_LIBS
      MTD_QT_LINK_LIBS
      PARENT_DEPENDENCIES
  )
  cmake_parse_arguments(PARSED "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
  if(PARSED_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR "Unexpected arguments found: ${PARSED_UNPARSED_ARGUMENTS}")
  endif()
  _append_qt_suffix(VERSION ${PARSED_QT_VERSION} OUTPUT_VARIABLE _target_name ${PARSED_TARGET_NAME})
  # test generation
  list(APPEND PARSED_SRC ${PARSED_QT${PARSED_QT_VERSION}_SRC})
  _append_qt_suffix(AS_DIR VERSION ${PARSED_QT_VERSION} OUTPUT_VARIABLE CXXTEST_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR})
  set(TESTHELPER_SRCS ${PARSED_TEST_HELPER_SRCS})
  cxxtest_add_test(${_target_name} ${PARSED_SRC})

  # Warning suppression
  _disable_suggest_override(${PARSED_QT_VERSION} ${_target_name})

  # libraries
  set(_link_libs ${PARSED_LINK_LIBS} ${_mtd_qt_libs})
  if(PARSED_QT_VERSION EQUAL 5)
    set(_link_libs Qt5::Widgets ${PARSED_QT5_LINK_LIBS} ${_link_libs})
  else()
    message(FATAL_ERROR "Unknown Qt version. Please specify only the major version.")
  endif()
  _append_qt_suffix(VERSION ${PARSED_QT_VERSION} OUTPUT_VARIABLE _mtd_qt_libs ${PARSED_MTD_QT_LINK_LIBS})

  # client and system headers
  target_include_directories(${_target_name} PRIVATE ${PARSED_INCLUDE_DIRS})
  target_include_directories(${_target_name} SYSTEM PRIVATE ${CXXTEST_INCLUDE_DIR})

  target_link_libraries(${_target_name} LINK_PRIVATE ${LINK_LIBS} ${_link_libs} ${_mtd_qt_libs})
  if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(${_target_name} PRIVATE -Wno-uninitialized)
  endif()

  # Add dependency to any parents
  foreach(_dep ${PARSED_PARENT_DEPENDENCIES})
    add_dependencies(${_dep} ${_target_name})
  endforeach()

  # set folder for Visual Studio
  set_target_properties(${_target_name} PROPERTIES FOLDER "Qt${PARSED_QT_VERSION}Tests")
endfunction()

# Given a list of arguments decide which Qt versions should be built. QT_VERSION can be specified to override this.
function(_qt_versions output_list)
  # process argument list
  list(FIND ARGN "QT_VERSION" _ver_idx)
  if(_ver_idx EQUAL -1)
    # default versions
    set(_qt_vers 5)
  else()
    math(EXPR _ver_value_idx "${_ver_idx}+1")
    list(GET ARGN ${_ver_value_idx} _ver_value)
    list(APPEND _qt_vers ${_ver_value})
  endif()
  if(NOT (ENABLE_WORKBENCH OR ENABLE_MANTIDQT))
    list(REMOVE_ITEM _qt_vers 5)
  endif()
  set(${output_list}
      ${_qt_vers}
      PARENT_SCOPE
  )
endfunction()

# Appends a string to the given variable to define the Qt version the library is built against. version: Version number
# of the library output_variable: The name of an output variable. This will be set on the parent scope option: as_dir If
# true then treat the inputs as directories at append a subdirectory ARGN: A list of strings to process
function(_append_qt_suffix)
  set(options AS_DIR)
  set(oneValueArgs VERSION OUTPUT_VARIABLE)
  cmake_parse_arguments(PARSED "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  if(PARSED_AS_DIR)
    set(_target_suffix "qt${PARSED_VERSION}")
    set(_sep "/")
  else()
    set(_target_suffix "Qt${PARSED_VERSION}")
    set(_sep "")
  endif()
  set(_out)
  foreach(_item ${PARSED_UNPARSED_ARGUMENTS})
    list(APPEND _out ${_item}${_sep}${_target_suffix})
  endforeach()
  set(${PARSED_OUTPUT_VARIABLE}
      ${_out}
      PARENT_SCOPE
  )
endfunction()

# Wrap generation of moc files We call the qt{5}_wrap_cpp individually for each file and force the include path to be
# absolute to avoid relative paths whose length exceed the maximum allowed limit on Windows (260 chars). It is assumed
# that the input paths can be made absolute by prefixing them with ${CMAKE_CURRENT_LIST_DIR} It assumes that the unnamed
# arguments are the input files to run through moc
function(_internal_qt_wrap_cpp qtversion moc_generated)
  set(options)
  set(oneValueArgs)
  set(multiValueArgs DEFS INFILES)
  cmake_parse_arguments(PARSED "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
  foreach(_def ${PARSED_DEFS})
    list(APPEND _moc_defs "-D${_def}")
  endforeach()
  foreach(_infile ${PARSED_INFILES})
    if(qtversion EQUAL 5)
      qt5_wrap_cpp(moc_generated ${_infile} OPTIONS -i -f${CMAKE_CURRENT_LIST_DIR}/${_infile} ${_moc_defs})
    else()
      message(FATAL_ERROR "Unknown Qt version='${qtversion}'.")
    endif()
  endforeach()
  # Pass the output variable out
  set(${moc_generated}
      ${${moc_generated}}
      PARENT_SCOPE
  )
endfunction()

# Disables suggest override for versions of Qt < 5.6.2 as Q_OBJECT produces them and the cannot be avoided.
function(_disable_suggest_override _qt_version _target)
  # For Qt < 5.6.2 gcc-5 produces warnings when it encounters the Q_OBJECT macro. We disable the warning in this case
  if(_qt_version EQUAL 5
     AND CMAKE_COMPILER_IS_GNUCXX
     AND Qt5Core_VERSION_STRING VERSION_LESS "5.6.2"
  )
    get_target_property(_options ${_target} COMPILE_OPTIONS)
    string(REPLACE "-Wsuggest-override" "" _options "${_options}")
    set_target_properties(${_target} PROPERTIES COMPILE_OPTIONS "${_options}")
  endif()
endfunction()
