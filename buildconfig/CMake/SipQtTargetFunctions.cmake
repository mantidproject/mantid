# Defines utility functions to help with generating
# targets that produce a Python module from a set
# of .sip definitions
#

#
# brief: Add a module target to generate Python bindings
# for a set of sip sources. The sources list should be a list of filenames
# without a path. The .sip module is generated in the CMAKE_CURRENT_BINARY_DIR
# keyword: MODULE_NAME The name of the final python module (without extension)
# keyword: MODULE_OUTPUT_DIR The final destination of the built module optional
# keyword: SIP_SRC_DIR The directory containing the given .sip files
# keyword: SIP_SRCS A list of input .sip file paths
# keyword: HEADER_DEPS A list of header files that are included
#                      in the .sip files. These are set as dependencies on
#                      the target.
# keyword: INCLUDE_DIRS A list of additional target_include_directories
# keyword: LINK_LIBS A list of additional target_link_libraries
# keyword: PYQT_VERSION A single value indicating the version of PyQt
#                       to compile against
function ( mtd_add_sip_module )
  find_file ( _sipmodule_template_path NAME sipqtmodule_template.sip.in
    PATHS ${CMAKE_MODULE_PATH} )
  if ( NOT _sipmodule_template_path )
    message ( FATAL "Unable to find sipqtmodule_template.sip.in in cmake module path. Cannot continue." )
  endif ()

  set ( options )
  set ( oneValueArgs MODULE_NAME TARGET_NAME MODULE_OUTPUT_DIR
                    SIP_SRC_DIR PYQT_VERSION FOLDER )
  set ( multiValueArgs SIP_SRCS HEADER_DEPS INCLUDE_DIRS LINK_LIBS )
  cmake_parse_arguments ( PARSED "${options}" "${oneValueArgs}"
                         "${multiValueArgs}" ${ARGN} )

  # Create the module spec file from the list of .sip files
  # The template file expects the variables to have certain names
  set ( MODULE_NAME ${PARSED_MODULE_NAME} )
  set ( SIP_INCLUDES )
  foreach ( _sip_file ${PARSED_SIP_SRCS} )
    set ( SIP_INCLUDES "${SIP_INCLUDES}%Include ${_sip_file}\n" )
  endforeach ()
  set ( _module_spec ${CMAKE_CURRENT_BINARY_DIR}/${PARSED_MODULE_NAME}.sip )
  configure_file ( ${_sipmodule_template_path} ${_module_spec} )

  # Run sip code generator
  set ( _pyqt_sip_dir ${PYQT${PARSED_PYQT_VERSION}_SIP_DIR} )
  set ( _pyqt_sip_flags ${PYQT${PARSED_PYQT_VERSION}_SIP_FLAGS} )
  set ( _sip_generated_cpp ${CMAKE_CURRENT_BINARY_DIR}/sip${PARSED_MODULE_NAME}part0.cpp )
  add_custom_command ( OUTPUT ${_sip_generated_cpp}

    COMMAND ${SIP_EXECUTABLE}
      -I ${PARSED_SIP_SRC_DIR} -I ${_pyqt_sip_dir}
      ${_pyqt_sip_flags}
      -c ${CMAKE_CURRENT_BINARY_DIR} -j1 -w -e ${_module_spec}
    DEPENDS ${_module_spec} ${SIP_SRCS} ${PARSED_HEADER_DEPS}
    COMMENT "Generating ${PARSED_MODULE_NAME} python bindings with sip"
  )

  add_library ( ${PARSED_TARGET_NAME} MODULE ${_sip_generated_cpp} )
  target_include_directories ( ${PARSED_TARGET_NAME} SYSTEM PRIVATE ${SIP_INCLUDE_DIR} )
  target_include_directories ( ${PARSED_TARGET_NAME} PRIVATE ${PARSED_INCLUDE_DIRS} )
  target_link_libraries ( ${PARSED_TARGET_NAME} PRIVATE ${PARSED_LINK_LIBS} )

  # Set all required properties on the target
  set_target_properties ( ${PARSED_TARGET_NAME} PROPERTIES
    LIBRARY_OUTPUT_NAME ${PARSED_MODULE_NAME} )

  if ( PARSED_MODULE_OUTPUT_DIR )
    set_target_properties ( ${PARSED_TARGET_NAME} PROPERTIES
      LIBRARY_OUTPUT_DIRECTORY ${PARSED_MODULE_OUTPUT_DIR} )
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
    if (OSX_VERSION VERSION_GREATER 10.8)
      set_target_properties ( ${PARSED_TARGET_NAME} PROPERTIES INSTALL_RPATH "@loader_path/../MacOS")
    endif ()
  else ()
    set_target_properties( ${PARSED_TARGET_NAME} PROPERTIES PREFIX "" )
  endif ()
endfunction()
