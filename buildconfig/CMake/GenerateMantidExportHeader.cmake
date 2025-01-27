include(GenerateExportHeader)
function(GENERATE_MANTID_EXPORT_HEADER TARGET_LIBRARY GENERATE_EXTERN)
  string(TOUPPER "${TARGET_LIBRARY}" TARGET_NAME)
  set(CUSTOM
      "\n\
#ifndef ${TARGET_NAME}_DEPRECATED\n\
    #define ${TARGET_NAME}_DEPRECATED(func) MANTID_${TARGET_NAME}_DEPRECATED func\n\
#endif\n\n"
  )

  if(GENERATE_EXTERN)
    set(CUSTOM
        "${CUSTOM}\
// MantidKernel/System.h will be removed\n\
#include \"MantidKernel/System.h\"\n\n\
// Use extern keyword in client code to suppress class template instantiation\n\
#ifdef ${TARGET_LIBRARY}_EXPORTS\n\
#define EXTERN_MANTID_${TARGET_NAME}\n\
#else\n\
// EXTERN_IMPORT is defined in MantidKernel/System.h\n
#define EXTERN_MANTID_${TARGET_NAME} EXTERN_IMPORT\n\
#endif /* ${TARGET_LIBRARY}_EXPORTS*/\n\n\
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
    "MANTID_${TARGET_NAME}"
    PREFIX_NAME
    ""
    EXPORT_FILE_NAME
    "Mantid${TARGET_LIBRARY}/DllConfig.h"
    EXPORT_MACRO_NAME
    "MANTID_${TARGET_NAME}_DLL"
    DEPRECATED_MACRO_NAME
    "MANTID_${TARGET_NAME}_DEPRECATED"
    CUSTOM_CONTENT_FROM_VARIABLE
    CUSTOM
  )
endfunction(GENERATE_MANTID_EXPORT_HEADER)
