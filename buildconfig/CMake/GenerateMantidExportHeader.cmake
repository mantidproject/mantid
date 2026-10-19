include(GenerateExportHeader)

# Generate the DllConfig.h for a target.
#
# generate_mantid_export_header(<target> <generate_extern> [GENERATE_UNUSED_ARG] [BASE_NAME <name>] [EXPORT_MACRO_NAME
# <name>] [EXPORT_FILE_NAME <path>] [EXTERN_MACRO_NAME <name>])
#
# By default the generated header follows the Framework convention: it is written to Mantid<target>/DllConfig.h and
# exports with MANTID_<TARGET>_DLL. Sub-packages that predate this function use different macro names and header
# locations, and pass the optional arguments to keep them.
#
# Whether the macro expands to dllexport or dllimport is decided by the target's DEFINE_SYMBOL property, which CMake
# defaults to <target>_EXPORTS.
function(GENERATE_MANTID_EXPORT_HEADER TARGET_LIBRARY GENERATE_EXTERN)
  set(options GENERATE_UNUSED_ARG)
  set(oneValueArgs BASE_NAME EXPORT_MACRO_NAME EXPORT_FILE_NAME EXTERN_MACRO_NAME)
  cmake_parse_arguments(PARSED "${options}" "${oneValueArgs}" "" ${ARGN})

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

  # generate_export_header picks dllexport vs dllimport from this same property
  get_target_property(EXPORT_CONDITION ${TARGET_LIBRARY} DEFINE_SYMBOL)
  if(NOT EXPORT_CONDITION)
    set(EXPORT_CONDITION "${TARGET_LIBRARY}_EXPORTS")
  endif()

  # A function-like convenience wrapper, e.g. KERNEL_DEPRECATED(func). It is distinct from, and expands to,
  # generate_export_header's own attribute macro <BASE_NAME>_DEPRECATED.
  set(CUSTOM
      "\n\
#ifndef ${TARGET_NAME}_DEPRECATED\n\
    #define ${TARGET_NAME}_DEPRECATED(func) ${PARSED_BASE_NAME}_DEPRECATED func\n\
#endif\n\n"
  )

  if(GENERATE_EXTERN)
    set(CUSTOM
        "${CUSTOM}\
// Use extern keyword in client code to suppress class template instantiation\n\
#ifdef ${EXPORT_CONDITION}\n\
#define ${PARSED_EXTERN_MACRO_NAME}\n\
#else\n\
#define ${PARSED_EXTERN_MACRO_NAME} extern\n\
#endif /* ${EXPORT_CONDITION} */\n\n\
 "
    )
  endif()

  # <BASE_NAME>_EXTERN_TEMPLATE(X<T>) declares an explicit instantiation that this library defines, with `template class
  # <EXPORT_MACRO_NAME> X<T>;`, and exports. Declare it in a header that every translation unit naming X<T> includes,
  # inside the library as well as outside it: with hidden visibility, a single implicit instantiation in the library
  # hides the exported symbols. MSVC does not allow extern together with dllexport, so the declaration is dropped inside
  # the library there.
  if(WIN32)
    set(CUSTOM
        "${CUSTOM}\
// Declare an explicit instantiation that this library defines and exports\n\
#ifdef ${EXPORT_CONDITION}\n\
#define ${PARSED_BASE_NAME}_EXTERN_TEMPLATE(...) static_assert(true)\n\
#else\n\
#define ${PARSED_BASE_NAME}_EXTERN_TEMPLATE(...) extern template class ${PARSED_EXPORT_MACRO_NAME} __VA_ARGS__\n\
#endif /* ${EXPORT_CONDITION} */\n\n"
    )
  else()
    set(CUSTOM
        "${CUSTOM}\
// Declare an explicit instantiation that this library defines and exports\n\
#define ${PARSED_BASE_NAME}_EXTERN_TEMPLATE(...) extern template class ${PARSED_EXPORT_MACRO_NAME} __VA_ARGS__\n\n"
    )
  endif()

  if(PARSED_GENERATE_UNUSED_ARG)
    set(CUSTOM
        "${CUSTOM}\
#ifndef UNUSED_ARG\n\
    #define UNUSED_ARG(x) (void) x;\n\
#endif\n\n\
#include <cstdint>\n"
    )

  endif()

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
