include(GenerateExportHeader)

# Generate the DllConfig.h for a target.
#
# generate_mantid_export_header(<target> <generate_extern> [BASE_NAME <name>] [EXPORT_MACRO_NAME <name>]
# [EXPORT_FILE_NAME <path>] [EXTERN_MACRO_NAME <name>] [DEPRECATED_MACRO_NAME <name>])
#
# By default the generated header follows the Framework convention: it is written to Mantid<target>/DllConfig.h and
# exports with MANTID_<TARGET>_DLL. Sub-packages that predate this function use different macro names and header
# locations, and pass the optional arguments to keep them.
#
# Whether the macro expands to dllexport or dllimport is decided by the target's DEFINE_SYMBOL property, which CMake
# defaults to <target>_EXPORTS.
function(GENERATE_MANTID_EXPORT_HEADER TARGET_LIBRARY GENERATE_EXTERN)
  set(oneValueArgs BASE_NAME EXPORT_MACRO_NAME EXPORT_FILE_NAME EXTERN_MACRO_NAME DEPRECATED_MACRO_NAME)
  cmake_parse_arguments(PARSED "" "${oneValueArgs}" "" ${ARGN})

  string(TOUPPER "${TARGET_LIBRARY}" TARGET_NAME)

  if(NOT PARSED_BASE_NAME)
    set(PARSED_BASE_NAME "MANTID_${TARGET_NAME}")
  endif()
  if(NOT PARSED_EXPORT_MACRO_NAME)
    set(PARSED_EXPORT_MACRO_NAME "${PARSED_BASE_NAME}_DLL")
  endif()
  if(NOT PARSED_EXPORT_FILE_NAME)
    set(PARSED_EXPORT_FILE_NAME "Mantid${TARGET_LIBRARY}/DllConfig.h")
  endif()
  if(NOT PARSED_EXTERN_MACRO_NAME)
    set(PARSED_EXTERN_MACRO_NAME "EXTERN_${PARSED_BASE_NAME}")
  endif()
  if(NOT PARSED_DEPRECATED_MACRO_NAME)
    set(PARSED_DEPRECATED_MACRO_NAME "${TARGET_NAME}_DEPRECATED")
  endif()

  # generate_export_header picks dllexport vs dllimport from this same property
  get_target_property(EXPORT_CONDITION ${TARGET_LIBRARY} DEFINE_SYMBOL)
  if(NOT EXPORT_CONDITION)
    set(EXPORT_CONDITION "${TARGET_LIBRARY}_EXPORTS")
  endif()

  set(CUSTOM
      "\n\
#ifndef ${PARSED_DEPRECATED_MACRO_NAME}\n\
    #define ${PARSED_DEPRECATED_MACRO_NAME}(func) ${PARSED_BASE_NAME}_DEPRECATED func\n\
#endif\n\n"
  )

  if(GENERATE_EXTERN)
    set(CUSTOM
        "${CUSTOM}\
// MantidKernel/System.h will be removed\n\
#include \"MantidKernel/System.h\"\n\n\
// Use extern keyword in client code to suppress class template instantiation\n\
#ifdef ${EXPORT_CONDITION}\n\
#define ${PARSED_EXTERN_MACRO_NAME}\n\
#else\n\
// EXTERN_IMPORT is defined in MantidKernel/System.h\n
#define ${PARSED_EXTERN_MACRO_NAME} EXTERN_IMPORT\n\
#endif /* ${EXPORT_CONDITION} */\n\n\
 "
    )
  else()
    # UNUSED_ARG is defined and cstdint is included in MantidKernel/System.h
    set(CUSTOM
        "${CUSTOM}\
#ifndef UNUSED_ARG\n\
    #define UNUSED_ARG(x) (void) x;\n\
#endif\n\n\
#include <cstdint>\n"
    )

  endif(GENERATE_EXTERN)

  generate_export_header(
    "${TARGET_LIBRARY}"
    BASE_NAME
    "${PARSED_BASE_NAME}"
    PREFIX_NAME
    ""
    EXPORT_FILE_NAME
    "${PARSED_EXPORT_FILE_NAME}"
    EXPORT_MACRO_NAME
    "${PARSED_EXPORT_MACRO_NAME}"
    DEPRECATED_MACRO_NAME
    "${PARSED_BASE_NAME}_DEPRECATED"
    CUSTOM_CONTENT_FROM_VARIABLE
    CUSTOM
  )
endfunction(GENERATE_MANTID_EXPORT_HEADER)
