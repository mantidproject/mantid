# - try to find Poco libraries and include files
# POCO_INCLUDE_DIR where to find Standard.hxx, etc.
# POCO_LIBRARIES libraries to link against
# POCO_FOUND If false, do not try to use POCO

find_path ( POCO_INCLUDE_DIR Poco/Poco.h )

find_library ( POCO_LIB_FOUNDATION NAMES PocoFoundation )
find_library ( POCO_LIB_UTIL NAMES PocoUtil )
find_library ( POCO_LIB_XML NAMES PocoXML )
find_library ( POCO_LIB_NET NAMES PocoNet )
find_library ( POCO_LIB_CRYPTO NAMES PocoCrypto )
find_library ( POCO_LIB_NETSSL NAMES PocoNetSSL )

find_library ( POCO_LIB_FOUNDATION_DEBUG NAMES PocoFoundationd )
find_library ( POCO_LIB_UTIL_DEBUG NAMES PocoUtild )
find_library ( POCO_LIB_XML_DEBUG NAMES PocoXMLd )
find_library ( POCO_LIB_NET_DEBUG NAMES PocoNetd )
find_library ( POCO_LIB_CRYPTO_DEBUG NAMES PocoCryptod )
find_library ( POCO_LIB_NETSSL_DEBUG NAMES PocoNetSSLd )

if ( POCO_LIB_FOUNDATION_DEBUG )

set ( POCO_LIBRARIES optimized ${POCO_LIB_FOUNDATION}
                     optimized ${POCO_LIB_UTIL}
                     optimized ${POCO_LIB_XML}
                     optimized ${POCO_LIB_NET}
                     optimized ${POCO_LIB_CRYPTO}
                     optimized ${POCO_LIB_NETSSL}
                     debug ${POCO_LIB_FOUNDATION_DEBUG}
                     debug ${POCO_LIB_UTIL_DEBUG}
                     debug ${POCO_LIB_XML_DEBUG}
                     debug ${POCO_LIB_NET_DEBUG}
                     debug ${POCO_LIB_CRYPTO_DEBUG}
                     debug ${POCO_LIB_NETSSL_DEBUG}
)

else ()

set ( POCO_LIBRARIES ${POCO_LIB_FOUNDATION}
                     ${POCO_LIB_UTIL}
                     ${POCO_LIB_XML}
                     ${POCO_LIB_NET}
                     ${POCO_LIB_CRYPTO}
                     ${POCO_LIB_NETSSL}
)

endif()

# handle the QUIETLY and REQUIRED arguments and set POCO_FOUND to TRUE if 
# all listed variables are TRUE
include ( FindPackageHandleStandardArgs )
find_package_handle_standard_args( Poco DEFAULT_MSG POCO_LIBRARIES POCO_INCLUDE_DIR )
