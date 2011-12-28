file(TO_CMAKE_PATH "${CPPCHECK_ROOT_DIR}" CPPCHECK_ROOT_DIR)
set(CPPCHECK_ROOT_DIR
	"${CPPCHECK_ROOT_DIR}"
	CACHE
	PATH
	"Path to search for cppcheck")

# cppcheck app bundles on Mac OS X are GUI, we want command line only
set(_oldappbundlesetting ${CMAKE_FIND_APPBUNDLE})
set(CMAKE_FIND_APPBUNDLE NEVER)

if(CPPCHECK_EXECUTABLE AND NOT EXISTS "${CPPCHECK_EXECUTABLE}")
	set(CPPCHECK_EXECUTABLE "notfound" CACHE PATH FORCE "")
endif()

# If we have a custom path, look there first.
if(CPPCHECK_ROOT_DIR)
	find_program(CPPCHECK_EXECUTABLE
		NAMES
		cppcheck
		cli
		PATHS
		"${CPPCHECK_ROOT_DIR}"
		PATH_SUFFIXES
		cli
		NO_DEFAULT_PATH)
endif()

find_program(CPPCHECK_EXECUTABLE NAMES cppcheck)

if(MSVC)
  set(CPPCHECK_TEMPLATE_ARG --template vs)
  set(CPPCHECK_FAIL_REGULAR_EXPRESSION "[(]error[)]")
  set(CPPCHECK_WARN_REGULAR_EXPRESSION "[(]style[)]")
elseif(CMAKE_COMPILER_IS_GNUCXX)
  set(CPPCHECK_TEMPLATE_ARG --template gcc)
  set(CPPCHECK_FAIL_REGULAR_EXPRESSION " error: ")
  set(CPPCHECK_WARN_REGULAR_EXPRESSION " style: ")
else()
  set(CPPCHECK_TEMPLATE_ARG --template gcc)
  set(CPPCHECK_FAIL_REGULAR_EXPRESSION " error: ")
  set(CPPCHECK_WARN_REGULAR_EXPRESSION " style: ")
endif()

if(CPPCHECK_EXECUTABLE OR CPPCHECK_MARK_AS_ADVANCED)
	mark_as_advanced(CPPCHECK_ROOT_DIR)
endif()

if(CPPCHECK_EXECUTABLE)
  if(NOT TARGET cppcheck)
    add_custom_target(cppcheck)
    set_target_properties(cppcheck PROPERTIES EXCLUDE_FROM_ALL TRUE)
  endif()
endif()

mark_as_advanced(CPPCHECK_EXECUTABLE)
set ( CPPCHECK_ARGS "--enable=all" CACHE STRING "Arguments for running cppcheck" )
set ( CPPCHECK_GENERATE_XML OFF CACHE BOOL "Generate xml output files from cppcheck" )

function(add_cppcheck _name)
  if(NOT TARGET ${_name})
    message(FATAL_ERROR
            "add_cppcheck given a target name that does not exist: '${_name}' !")
  endif()

  if(CPPCHECK_EXECUTABLE)
    get_target_property(_cppcheck_sources "${_name}" SOURCES)
    set(_files)
    foreach(_source ${_cppcheck_sources})
      get_source_file_property(_cppcheck_lang "${_source}" LANGUAGE)
      get_source_file_property(_cppcheck_loc "${_source}" LOCATION)
      if("${_cppcheck_lang}" MATCHES "CXX")
        list(APPEND _files "${_cppcheck_loc}")
      endif()
    endforeach()

    if (CPPCHECK_GENERATE_XML )
      add_custom_target( cppcheck_${_name}
                         COMMAND ${CPPCHECK_EXECUTABLE} ${CPPCHECK_ARGS} --xml ${_files} 2> ${CMAKE_BINARY_DIR}/cppcheck-${_name}.xml 
                         DEPENDS ${_files}
                         COMMENT "cppcheck_${_name}: Running cppcheck to generate cppcheck-${_name}.xml"
                         )
    else (CPPCHECK_GENERATE_XML )
      add_custom_target( cppcheck_${_name}
                         COMMAND ${CPPCHECK_EXECUTABLE} ${CPPCHECK_ARGS} ${_files}
                         DEPENDS ${_files}
                         COMMENT "cppcheck_${_name}: Running cppcheck on ${_name} source files"
                         )
    endif (CPPCHECK_GENERATE_XML )
    add_dependencies ( cppcheck cppcheck_${_name} )

  endif()

endfunction()
