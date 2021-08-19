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

function( add_poco_lib POCO_COMPONENT POCO_LIB_MODULE POCO_DEBUG_LIB_MODULE )
  # Add poco library to list and also the corresponding debug library if it is available

  if ( EXISTS "${POCO_DEBUG_LIB_MODULE}" AND NOT ${CONDA_BUILD})
    set ( POCO_LIBRARIES ${POCO_LIBRARIES}
        optimized ${POCO_LIB_MODULE}
        debug ${POCO_DEBUG_LIB_MODULE}
        PARENT_SCOPE)
  else ()
    set ( POCO_LIBRARIES ${POCO_LIBRARIES} ${POCO_LIB_MODULE} PARENT_SCOPE)
  endif()

  if(NOT TARGET Poco::${POCO_COMPONENT})
  add_library(Poco::${POCO_COMPONENT} UNKNOWN IMPORTED)
  set_target_properties(Poco::${POCO_COMPONENT} PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${POCO_INCLUDE_DIR}
  IMPORTED_LOCATION ${POCO_LIB_MODULE}
  )
  if ( EXISTS "${POCO_DEBUG_LIB_MODULE}" AND NOT ${CONDA_BUILD})
  set_target_properties(Poco::${POCO_COMPONENT} PROPERTIES
  IMPORTED_LOCATION_DEBUG ${POCO_DEBUG_LIB_MODULE}
  )
  endif()

  endif()

endfunction( add_poco_lib )

add_poco_lib( Foundation ${POCO_LIB_FOUNDATION} ${POCO_LIB_FOUNDATION_DEBUG} )
add_poco_lib( Util ${POCO_LIB_UTIL} ${POCO_LIB_UTIL_DEBUG} )
add_poco_lib( XML ${POCO_LIB_XML} ${POCO_LIB_XML_DEBUG} )
add_poco_lib( Net ${POCO_LIB_NET} ${POCO_LIB_NET_DEBUG} )
add_poco_lib( Crypto ${POCO_LIB_CRYPTO} ${POCO_LIB_CRYPTO_DEBUG} )
add_poco_lib( NetSSL ${POCO_LIB_NETSSL} ${POCO_LIB_NETSSL_DEBUG} )

# Set a version string by examining either the Poco/Version.h header or
# the Poco/Foundation.h header if Version.h does not exist
if( POCO_INCLUDE_DIR )
  if ( EXISTS ${POCO_INCLUDE_DIR}/Poco/Version.h )
    set ( VERSION_FILE ${POCO_INCLUDE_DIR}/Poco/Version.h )
  else ()
    set ( VERSION_FILE ${POCO_INCLUDE_DIR}/Poco/Foundation.h )
  endif ()
  # regex quantifiers like {8} don't seem to work so we'll stick with + even though
  # it's not strictly true
  set ( VERS_REGEX "^#define[ \t]+POCO_VERSION[ \t]+0x([0-9A-F]+)$" )
  file ( STRINGS ${VERSION_FILE} POCO_VERSION REGEX ${VERS_REGEX} )
  # pull out just the part after the 0x
  string( REGEX REPLACE ${VERS_REGEX} "\\1" POCO_VERSION ${POCO_VERSION} )

  # Pretty format
  string( SUBSTRING ${POCO_VERSION} 0 2 POCO_VERSION_MAJOR )
  math(EXPR POCO_VERSION_MAJOR "0x${POCO_VERSION_MAJOR}" OUTPUT_FORMAT DECIMAL)
  string( REGEX REPLACE "^0\(.\)" "\\1" POCO_VERSION_MAJOR ${POCO_VERSION_MAJOR} )
  string( SUBSTRING ${POCO_VERSION} 2 2 POCO_VERSION_MINOR )
  math(EXPR POCO_VERSION_MINOR "0x${POCO_VERSION_MINOR}" OUTPUT_FORMAT DECIMAL)
  string( REGEX REPLACE "^0\(.\)" "\\1" POCO_VERSION_MINOR ${POCO_VERSION_MINOR} )
  string( SUBSTRING ${POCO_VERSION} 4 2 POCO_VERSION_PATCH )
  math(EXPR POCO_VERSION_PATCH "0x${POCO_VERSION_PATCH}" OUTPUT_FORMAT DECIMAL)
  string( REGEX REPLACE "^0\(.\)" "\\1" POCO_VERSION_PATCH ${POCO_VERSION_PATCH} )
  set ( POCO_VERSION "${POCO_VERSION_MAJOR}.${POCO_VERSION_MINOR}.${POCO_VERSION_PATCH}" )
endif()

# Also set a shared library version number. This is different to the main version number
# and can form part of the package name on some Linux systems
if( POCO_LIB_FOUNDATION )
  set ( POCO_SOLIB_VERSION "" )
  # The library path is usually a symlink to the actually library
  get_filename_component ( POCO_REAL_LIB_FOUNDATION ${POCO_LIB_FOUNDATION} REALPATH )
  set ( _LIB_REGEX "^.*.so.([0-9]+)$" )
  string( REGEX REPLACE ${_LIB_REGEX} "\\1" POCO_SOLIB_VERSION ${POCO_REAL_LIB_FOUNDATION} )
endif()


# handle the QUIETLY and REQUIRED arguments and set POCO_FOUND to TRUE if
# all listed variables are TRUE
include ( FindPackageHandleStandardArgs )
if (POCO_VERSION)
  find_package_handle_standard_args( Poco REQUIRED_VARS POCO_LIBRARIES POCO_INCLUDE_DIR
                                     VERSION_VAR POCO_VERSION )
else ()
  message (status "Failed to determine Poco version: Ignoring requirement")
  find_package_handle_standard_args( Poco DEFAULT_MSG POCO_LIBRARIES POCO_INCLUDE_DIR )
endif ()

mark_as_advanced ( POCO_INCLUDE_DIR
                   POCO_LIB_FOUNDATION POCO_LIB_FOUNDATION_DEBUG
                   POCO_LIB_XML POCO_LIB_XML_DEBUG
                   POCO_LIB_UTIL POCO_LIB_UTIL_DEBUG
                   POCO_LIB_NET POCO_LIB_NET_DEBUG
                   POCO_LIB_CRYPTO POCO_LIB_CRYPTO_DEBUG
                   POCO_LIB_NETSSL POCO_LIB_NETSSL_DEBUG
)
