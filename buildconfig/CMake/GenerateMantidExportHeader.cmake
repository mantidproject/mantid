include (GenerateExportHeader)
function( GENERATE_MANTID_EXPORT_HEADER TARGET_LIBRARY )
    string(TOUPPER "${TARGET_LIBRARY}" TARGET_NAME)
    set (CUSTOM "\n")
    
    if(MSVC)
        set (CUSTOM "${CUSTOM}\n\
#ifdef _WIN32\n\
    // 'identifier' : class 'type' needs to have dll-interface to be used by clients\n\
    // of class 'type2'\n\
    // Things from the std library give these warnings and we can't do anything\n\
    // about them.\n\
    #pragma warning(disable : 4251)\n\
    // Given that we are compiling everything with msvc under Windows and\n\
    // linking all with the same runtime we can disable the warning about\n\
    // inheriting from a non-exported interface, e.g. std::runtime_error */\n\
    #pragma warning(disable : 4275)\n\
    // Warning C4373: previous versions of the compiler did not override when\n\
    // parameters only differed by const/volatile qualifiers\n\
    // This is basically saying that it now follows the C++ standard and doesn't\n\
    // seem useful\n\
    #pragma warning(disable : 4373)\n\
#endif\n\n\
")
    endif(MSVC)
    
    set(CUSTOM "${CUSTOM}\
#ifndef UNUSED_ARG\n\
    #define UNUSED_ARG(x) (void) x;\n\
#endif\n\n\
#ifndef ${TARGET_NAME}_DEPRECATED\n\
    #define ${TARGET_NAME}_DEPRECATED(func) MANTID_${TARGET_NAME}_DEPRECATED func\n\
#endif\n\n\
 ")
    
    generate_export_header("${TARGET_LIBRARY}" 
       BASE_NAME "MANTID_${TARGET_NAME}" 
       PREFIX_NAME ""
       EXPORT_FILE_NAME "Mantid${TARGET_LIBRARY}/DllConfig.h"
       EXPORT_MACRO_NAME "MANTID_${TARGET_NAME}_DLL"
       DEPRECATED_MACRO_NAME "MANTID_${TARGET_NAME}_DEPRECATED"
       CUSTOM_CONTENT_FROM_VARIABLE CUSTOM
    )
endfunction( GENERATE_MANTID_EXPORT_HEADER )
