# Defines utility functions to help with generating
# targets that produce a Python module from a set
# of .sip definitions
include ( QtTargetFunctions )


# brief: Add a module target to generate Python bindings
# for a set of sip sources. The sources list should be a list of filenames
# without a path. The .sip module is generated in the CMAKE_CURRENT_BINARY_DIR
# keyword: MODULE_NAME The name of the final python module (without extension)
# keyword: MODULE_OUTPUT_DIR The final destination of the built module optional
# keyword: SIP_SRCS A list of input .sip file paths
# keyword: HEADER_DEPS A list of header files that are included
#                      in the .sip files. These are set as dependencies on
#                      the target.
# keyword: INCLUDE_DIRS A list of additional target_include_directories
# keyword: LINK_LIBS A list of additional target_link_libraries
# keyword: PYQT_VERSION A single value indicating the version of PyQt
#                       to compile against
# keyword: INSTALL_DIR The target location for installing this library
# keyword: OSX_INSTALL_RPATH Install path for osx version > 10.8
# keyword: LINUX_INSTALL_RPATH Install path for CMAKE_SYSTEM_NAME == Linux
function ( mtd_add_sip_module )
  find_file ( _sipmodule_template_path NAME sipqtmodule_template.sip.in
    PATHS ${CMAKE_MODULE_PATH} )
  if ( NOT _sipmodule_template_path )
    message ( FATAL "Unable to find sipqtmodule_template.sip.in in cmake module path. Cannot continue." )
  endif ()

  set ( options )
  set ( oneValueArgs MODULE_NAME TARGET_NAME MODULE_OUTPUT_DIR
                     PYQT_VERSION FOLDER )
  set ( multiValueArgs SIP_SRCS HEADER_DEPS INCLUDE_DIRS LINK_LIBS INSTALL_DIR OSX_INSTALL_RPATH LINUX_INSTALL_RPATH )
  cmake_parse_arguments ( PARSED "${options}" "${oneValueArgs}"
                         "${multiValueArgs}" ${ARGN} )

  # Create the module spec file from the list of .sip files
  # The template file expects the variables to have certain names
  set ( MODULE_NAME ${PARSED_MODULE_NAME} )
  set ( SIP_INCLUDES )
  # Sip cannot %Include absolute paths so make them relative and add
  # -I flag
  foreach ( _sip_file ${PARSED_SIP_SRCS} )
    get_filename_component ( _filename ${_sip_file} NAME )
    get_filename_component ( _directory ${_sip_file} DIRECTORY )
    set ( SIP_INCLUDES "${SIP_INCLUDES}%Include ${_filename}\n" )
    if ( NOT _directory )
      set ( _directory ${CMAKE_CURRENT_LIST_DIR} )
    elseif ( NOT IS_ABSOLUTE _directory )
      set ( _directory ${CMAKE_CURRENT_LIST_DIR}/${_directory} )
    endif()
    list ( APPEND _sip_include_flags "-I${_directory}" )
    list ( APPEND _sip_include_deps "${_sip_file}" )
  endforeach ()

  # Add absolute paths for header dependencies
  foreach ( _header ${PARSED_HEADER_DEPS} )
    if ( IS_ABSOLUTE ${_header} )
      list ( APPEND _sip_include_deps "${_header}" )
    else ()
      list ( APPEND _sip_include_deps "${CMAKE_CURRENT_LIST_DIR}/${_header}" )
    endif()
  endforeach ()

  # Run sip code generator
  set ( _module_spec ${CMAKE_CURRENT_BINARY_DIR}/${PARSED_MODULE_NAME}.sip )
  configure_file ( ${_sipmodule_template_path} ${_module_spec} )
  set ( _pyqt_sip_dir ${PYQT${PARSED_PYQT_VERSION}_SIP_DIR} )
  list ( APPEND _sip_include_flags "-I${_pyqt_sip_dir}" )
  set ( _pyqt_sip_flags ${PYQT${PARSED_PYQT_VERSION}_SIP_FLAGS} )
  set ( _sip_generated_cpp ${CMAKE_CURRENT_BINARY_DIR}/sip${PARSED_MODULE_NAME}part0.cpp )
  add_custom_command ( OUTPUT ${_sip_generated_cpp}
    COMMAND ${SIP_EXECUTABLE}
      ${_sip_include_flags} ${_pyqt_sip_flags}
      -c ${CMAKE_CURRENT_BINARY_DIR} -j1 -w -e ${_module_spec}
    DEPENDS ${_module_spec} ${_sip_include_deps} ${SIP_INCLUDE_DIR}/sip.h
    COMMENT "Generating ${PARSED_MODULE_NAME} python bindings with sip"
  )

  add_library ( ${PARSED_TARGET_NAME} MODULE ${_sip_generated_cpp} ${_sip_include_deps} )
  if ( ${CMAKE_SYSTEM_NAME} MATCHES "Darwin" )
    prune_usr_local_include ( ${PARSED_LINK_LIBS} )
    if ( NOT (SIP_INCLUDE_DIR STREQUAL /usr/local/include) )
      target_include_directories ( ${PARSED_TARGET_NAME} SYSTEM PRIVATE ${SIP_INCLUDE_DIR} )
    endif ()
  else()
    target_include_directories ( ${PARSED_TARGET_NAME} SYSTEM PRIVATE ${SIP_INCLUDE_DIR} )
  endif ()
  target_include_directories ( ${PARSED_TARGET_NAME} PRIVATE ${PARSED_INCLUDE_DIRS} )
  target_link_libraries ( ${PARSED_TARGET_NAME} PRIVATE ${PARSED_LINK_LIBS} )

  # Set all required properties on the target
  set_target_properties ( ${PARSED_TARGET_NAME} PROPERTIES
    LIBRARY_OUTPUT_NAME ${PARSED_MODULE_NAME} )
  set_target_properties ( ${PARSED_TARGET_NAME} PROPERTIES CXX_CLANG_TIDY "" )

  if ( PARSED_MODULE_OUTPUT_DIR )
    set_target_properties ( ${PARSED_TARGET_NAME} PROPERTIES
      LIBRARY_OUTPUT_DIRECTORY ${PARSED_MODULE_OUTPUT_DIR} )
  endif ()

  if (OSX_VERSION VERSION_GREATER 10.8)
    if (PARSED_OSX_INSTALL_RPATH)
      set_target_properties ( ${PARSED_TARGET_NAME} PROPERTIES INSTALL_RPATH  "${PARSED_OSX_INSTALL_RPATH}" )
    endif()
  elseif ( ${CMAKE_SYSTEM_NAME} STREQUAL "Linux" )
    if (PARSED_LINUX_INSTALL_RPATH)
      set_target_properties ( ${PARSED_TARGET_NAME} PROPERTIES INSTALL_RPATH  "${PARSED_LINUX_INSTALL_RPATH}" )
    endif ()
  endif ()

  if ( PARSED_INSTALL_DIR AND ENABLE_WORKBENCH)
    mtd_install_qt_library ( ${PARSED_PYQT_VERSION} ${PARSED_TARGET_NAME} "" ${PARSED_INSTALL_DIR} )
  endif ()

  if ( WIN32 )
    set_target_properties( ${PARSED_TARGET_NAME} PROPERTIES PREFIX "" SUFFIX ".pyd" )
    if ( PYTHON_DEBUG_LIBRARY )
      set_target_properties ( ${PARSED_TARGET_NAME} PROPERTIES
        LIBRARY_OUTPUT_NAME_DEBUG ${PARSED_TARGET_NAME}_d )
    endif ()
    if ( MSVC_IDE )
      set_target_properties( ${PARSED_TARGET_NAME} PROPERTIES FOLDER "${PARSED_FOLDER}" )
    endif ()
  elseif ( APPLE )
    set_target_properties( ${PARSED_TARGET_NAME} PROPERTIES PREFIX "" SUFFIX ".so" )
  else ()
    set_target_properties( ${PARSED_TARGET_NAME} PROPERTIES PREFIX "" )
  endif ()
endfunction()
