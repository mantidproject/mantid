# Defines utility functions to help with generating targets that produce a
# Python module from a set of .sip definitions
include(QtTargetFunctions)

# ~~~
# brief: Add a module target to generate Python bindings for a set of sip
# sources. The sources list should be a list of filenames without a path. The
# .sip module is generated in the CMAKE_CURRENT_BINARY_DIR
# keyword: MODULE_NAME The name of the final python module (without extension)
# keyword: MODULE_OUTPUT_DIR The final destination of the built module optional
# keyword: SIP_SRC The input .sip file path
# keyword: HEADER_DEPS A list of header files that are included in the .sip files. These are set as dependencies on
# the target.
# keyword: INCLUDE_DIRS A list of additional target_include_directories
# keyword: SYSTEM_INCLUDE_DIRS A list of additional target_include_directories that should be marked as system headers
# keyword: LINK_LIBS A list of additional target_link_libraries
# keyword: PYQT_VERSION A single value indicating the version of PyQt to compile against
# keyword: INSTALL_DIR The target location for installing this library
# keyword: OSX_INSTALL_RPATH Install path for osx version > 10.8
# keyword: LINUX_INSTALL_RPATH Install path for CMAKE_SYSTEM_NAME == Linux
# ~~~
function(mtd_add_sip_module)
  set(options)
  set(oneValueArgs MODULE_NAME TARGET_NAME SIP_SRC MODULE_OUTPUT_DIR
                   PYQT_VERSION FOLDER
  )
  set(multiValueArgs
      HEADER_DEPS
      SYSTEM_INCLUDE_DIRS
      INCLUDE_DIRS
      LINK_LIBS
      INSTALL_DIR
      OSX_INSTALL_RPATH
      LINUX_INSTALL_RPATH
  )
  cmake_parse_arguments(
    PARSED "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN}
  )

  # Create the module spec file from the list of .sip files The template file
  # expects the variables to have certain names
  set(MODULE_NAME ${PARSED_MODULE_NAME})
  # Add absolute paths for target header dependencies
  foreach(_header ${PARSED_HEADER_DEPS})
    if(IS_ABSOLUTE ${_header})
      list(APPEND _sip_include_deps "${_header}")
    else()
      list(APPEND _sip_include_deps "${CMAKE_CURRENT_LIST_DIR}/${_header}")
    endif()
  endforeach()

  # Configure final module
  set(_module_spec ${CMAKE_CURRENT_LIST_DIR}/${PARSED_SIP_SRC})
  if(PARSED_PYQT_VERSION EQUAL 5)
    if(SIP_BUILD_EXECUTABLE)
      _add_sip_library(
        ${PARSED_TARGET_NAME} ${PARSED_MODULE_NAME} ${_module_spec}
        ${PARSED_PYQT_VERSION} _sip_include_deps
      )
    else()
      _add_sip_library_v4(
        ${PARSED_TARGET_NAME} ${PARSED_MODULE_NAME} ${_module_spec}
        _sip_include_deps
      )
    endif()
  elseif(PARSED_PYQT_VERSION EQUAL 6)
    message(FATAL_ERROR "PyQt6 is not yet supported")
  else()
    message(FATAL_ERROR "Unknown PYQT_VERSION: ${PARSED_PYQT_VERSION}")
  endif()

  # Suppress Warnings about sip bindings have PyObject -> PyFunc casts which is
  # a valid pattern GCC8 onwards detects GCC 8 onwards needs to disable
  # functional casting at the Python interface
  target_compile_options(
    ${PARSED_TARGET_NAME}
    PRIVATE
      $<$<AND:$<CXX_COMPILER_ID:GNU>,$<VERSION_GREATER_EQUAL:$<CXX_COMPILER_VERSION>,8.0>>:-Wno-cast-function-type>
  )
  target_include_directories(
    ${PARSED_TARGET_NAME} PRIVATE ${PARSED_INCLUDE_DIRS}
  )
  target_include_directories(
    ${PARSED_TARGET_NAME} SYSTEM PRIVATE ${PARSED_SYSTEM_INCLUDE_DIRS} ${Python_INCLUDE_DIRS}
  )

  if(USE_PYTHON_DYNAMIC_LIB)
  target_link_libraries(${PARSED_TARGET_NAME} PRIVATE Python::Python)
  endif()

  target_link_libraries(
    ${PARSED_TARGET_NAME} PRIVATE ${PARSED_LINK_LIBS}
  )

  # Set all required properties on the target
  set_target_properties(
    ${PARSED_TARGET_NAME} PROPERTIES LIBRARY_OUTPUT_NAME ${PARSED_MODULE_NAME}
  )
  set_target_properties(${PARSED_TARGET_NAME} PROPERTIES CXX_CLANG_TIDY "")

  if(PARSED_MODULE_OUTPUT_DIR)
    set_target_properties(
      ${PARSED_TARGET_NAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY
                                       ${PARSED_MODULE_OUTPUT_DIR}
    )
  endif()

  if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    if(PARSED_OSX_INSTALL_RPATH)
      set_target_properties(
        ${PARSED_TARGET_NAME} PROPERTIES INSTALL_RPATH
                                         "${PARSED_OSX_INSTALL_RPATH}"
      )
    endif()
  elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    if(PARSED_LINUX_INSTALL_RPATH)
      set_target_properties(
        ${PARSED_TARGET_NAME} PROPERTIES INSTALL_RPATH
                                         "${PARSED_LINUX_INSTALL_RPATH}"
      )
    endif()
  endif()

  if(PARSED_INSTALL_DIR AND (ENABLE_WORKBENCH OR MANTID_QT_LIB STREQUAL "BUILD"))
    mtd_install_qt_library(
      ${PARSED_PYQT_VERSION} ${PARSED_TARGET_NAME} "" ${PARSED_INSTALL_DIR}
    )
  endif()

  if(WIN32)
    set_target_properties(
      ${PARSED_TARGET_NAME} PROPERTIES PREFIX "" SUFFIX ".pyd"
    )
    if(PYTHON_DEBUG_LIBRARY)
      set_target_properties(
        ${PARSED_TARGET_NAME} PROPERTIES LIBRARY_OUTPUT_NAME_DEBUG
                                         ${PARSED_TARGET_NAME}_d
      )
    endif()
    if(MSVC_IDE)
      set_target_properties(
        ${PARSED_TARGET_NAME} PROPERTIES FOLDER "${PARSED_FOLDER}"
      )
    endif()
  elseif(APPLE)
    set_target_properties(
      ${PARSED_TARGET_NAME} PROPERTIES PREFIX "" SUFFIX ".so"
    )
  else()
    set_target_properties(${PARSED_TARGET_NAME} PROPERTIES PREFIX "")
  endif()
endfunction()

# Private API Add a library target based on the given sip module file.
# ~~~
# Add a library target based on the given sip module file. The library target
# will first generate the bindings and then compile to code.
# Args:
#   - target_name: The name of the library target
#   - module_name: The name of the sip module as seen by Python
#   - module_spec: The full path to the sip module file
#   - pyqt_major_version: The major version of PyQt building against
#   - sip_include_deps_var: A variable containing a list of files to add as
#                           dependencies to the target
# ~~~
function(_add_sip_library target_name module_name module_spec
         pyqt_major_version sip_include_deps_var
)
  # first produce template files for the sip-build project in the binary
  # directory
  set(_project_dir ${CMAKE_CURRENT_BINARY_DIR})

  # replacement variables for templates
  set(MODULE_NAME ${module_name})
  set(MODULE_SPEC_FILE ${module_spec})
  set(PYQT_MAJOR_VERSION ${pyqt_major_version})
  set(PYQT_SIP_ABI_VERSION ${PYQT${pyqt_major_version}_SIP_ABI_VERSION})

  # generate project files for sip-build
  configure_file(${SIP_PROJECT_PY_TEMPLATE} ${_project_dir}/project.py)
  configure_file(${SIP_PYPROJECT_TOML_TEMPLATE} ${_project_dir}/pyproject.toml)

  # add command for running sip-build. Sets a custom build directory and
  # sip-build is then responsible for the directory layout below that. We are
  # just interested in the directory that contains the generated .cpp file
  set(_sip_build_dir sip-generated)
  set(_sip_generated_cpp
      ${_project_dir}/${_sip_build_dir}/${module_name}/sip${module_name}part0.cpp
  )
  # We also have to deal with the added complication that sip generates code
  # that is not C++-17 compatible as it includes throw specifiers. We deal with
  # this by replacing them in the generated code.
  set(_sip_sanitizer ${CMAKE_SOURCE_DIR}/tools/sip/sip-sanitize-module.py)
  add_custom_command(
    OUTPUT ${_sip_generated_cpp}
    COMMAND ${SIP_BUILD_EXECUTABLE} ARGS --build-dir=${_sip_build_dir}
    COMMAND ${Python_EXECUTABLE} ${_sip_sanitizer} ARGS ${_sip_generated_cpp}
    WORKING_DIRECTORY ${_project_dir}
    DEPENDS ${module_spec} ${_project_dir}/project.py
            ${_project_dir}/pyproject.toml ${_sip_include_deps}
            ${_sip_sanitizer}
    COMMENT "Generating ${module_name} python bindings with sip"
  )

  add_library(
    ${target_name} MODULE ${_sip_generated_cpp} ${${_sip_include_deps_var}}
  )
  target_include_directories(
    ${target_name} SYSTEM PRIVATE ${_project_dir}/${_sip_build_dir}
  )

endfunction()

# ~~~
# Add a library target based on the given sip module file. The library target
# will first generate the bindings and then compile to code.
# Note that this is for sip <= v4 build system and hardcode to PyQt5
# Args:
#   - target_name: The name of the library target
#   - module_name: The name of the sip module as seen by Python
#   - module_spec: The full path to the sip module file
#   - sip_include_deps_var: A variable containing a list of files to add as
#                           dependencies to the target
# ~~~
function(_add_sip_library_v4 target_name module_name module_spec
         sip_include_deps_var
)
  if(NOT PYQT5_SIP_DIR)
    message(
      FATAL_ERROR
        "find_package(PyQt) must have been called with the correct PyQt version"
    )
  endif()

  # Build sip command
  list(APPEND _sip_include_flags "-I${PYQT5_SIP_DIR}")
  set(_pyqt_sip_flags "${PYQT5_SIP_FLAGS}")
  set(_sip_generated_cpp ${CMAKE_CURRENT_BINARY_DIR}/sip${module_name}part0.cpp)
  # We also have to deal with the added complication that sip generates code
  # that is not C++-17 compatible as it includes throw specifiers. We deal with
  # this by replacing them in the generated code.
  set(_sip_sanitizer ${CMAKE_SOURCE_DIR}/tools/sip/sip-sanitize-module.py)
  add_custom_command(
    OUTPUT ${_sip_generated_cpp}
    COMMAND ${SIP_EXECUTABLE} ARGS ${_sip_include_flags} ${_pyqt_sip_flags} -c
            ${CMAKE_CURRENT_BINARY_DIR} -j1 -w -e ${_module_spec}
    COMMAND ${Python_EXECUTABLE} ${_sip_sanitizer} ARGS ${_sip_generated_cpp}
    DEPENDS ${_module_spec} ${_sip_include_deps}
    COMMENT "Generating ${PARSED_MODULE_NAME} python bindings with sip"
  )

  add_library(
    ${target_name} MODULE ${_sip_generated_cpp} ${${_sip_include_deps_var}}
  )
  target_include_directories(${target_name} SYSTEM PRIVATE ${SIP_INCLUDE_DIR})
endfunction()
