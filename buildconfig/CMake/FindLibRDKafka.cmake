# * Try to find LibRDKafka headers and libraries.
#
# Usage of this module as follows:
#
# find_package(LibRDKafka)
#
# A minimum required version can also be specified, for example:
#
# find_package(LibRDKafka 0.11.1)
#
# Variables used by this module, they can change the default behaviour and need to be set before calling find_package:
#
# LibRDKafka_ROOT_DIR  Set this variable to the root installation of LibRDKafka if the module has problems finding the
# proper installation path.
#
# Variables defined by this module:
#
# LIBRDKAFKA_FOUND              System has LibRDKafka libs/headers LibRDKafka_LIBRARIES          The LibRDKafka
# libraries LibRDKafka_INCLUDE_DIR The location of LibRDKafka headers

macro(HEXCHAR2DEC VAR VAL)
  if(${VAL} MATCHES "[0-9]")
    set(${VAR} ${VAL})
  elseif(${VAL} MATCHES "[aA]")
    set(${VAR} 10)
  elseif(${VAL} MATCHES "[bB]")
    set(${VAR} 11)
  elseif(${VAL} MATCHES "[cC]")
    set(${VAR} 12)
  elseif(${VAL} MATCHES "[dD]")
    set(${VAR} 13)
  elseif(${VAL} MATCHES "[eE]")
    set(${VAR} 14)
  elseif(${VAL} MATCHES "[fF]")
    set(${VAR} 15)
  else()
    message(FATAL_ERROR "Invalid format for hexadecimal character")
  endif()

endmacro(HEXCHAR2DEC)

macro(HEX2DEC VAR VAL)

  set(CURINDEX 0)
  string(LENGTH "${VAL}" CURLENGTH)

  set(${VAR} 0)

  while(CURINDEX LESS CURLENGTH)

    string(SUBSTRING "${VAL}" ${CURINDEX} 1 CHAR)

    hexchar2dec(CHAR ${CHAR})

    math(EXPR POWAH "(1<<((${CURLENGTH}-${CURINDEX}-1)*4))")
    math(EXPR CHAR "(${CHAR}*${POWAH})")
    math(EXPR ${VAR} "${${VAR}}+${CHAR}")
    math(EXPR CURINDEX "${CURINDEX}+1")
  endwhile()

endmacro(HEX2DEC)

macro(HEX2DECVERSION DECVERSION HEXVERSION)

  string(SUBSTRING "${HEXVERSION}" 0 2 MAJOR_HEX)
  hex2dec(MAJOR_DEC ${MAJOR_HEX})

  string(SUBSTRING "${HEXVERSION}" 2 2 MINOR_HEX)
  hex2dec(MINOR_DEC ${MINOR_HEX})

  string(SUBSTRING "${HEXVERSION}" 4 2 REVISION_HEX)
  hex2dec(REVISION_DEC ${REVISION_HEX})

  set(${DECVERSION} ${MAJOR_DEC}.${MINOR_DEC}.${REVISION_DEC})

endmacro(HEX2DECVERSION)

if(NOT LibRDKafka_FIND_VERSION)
  if(NOT LibRDKafka_FIND_VERSION_MAJOR)
    set(LibRDKafka_FIND_VERSION_MAJOR 0)
  endif(NOT LibRDKafka_FIND_VERSION_MAJOR)
  if(NOT LibRDKafka_FIND_VERSION_MINOR)
    set(LibRDKafka_FIND_VERSION_MINOR 11)
  endif(NOT LibRDKafka_FIND_VERSION_MINOR)
  if(NOT LibRDKafka_FIND_VERSION_PATCH)
    set(LibRDKafka_FIND_VERSION_PATCH 0)
  endif(NOT LibRDKafka_FIND_VERSION_PATCH)

  set(LibRDKafka_FIND_VERSION
      "${LibRDKafka_FIND_VERSION_MAJOR}.${LibRDKafka_FIND_VERSION_MINOR}.${LibRDKafka_FIND_VERSION_PATCH}"
  )
endif(NOT LibRDKafka_FIND_VERSION)

macro(_LibRDKafka_check_version)
  file(READ "${LibRDKafka_INCLUDE_DIR}/librdkafka/rdkafka.h" _LibRDKafka_version_header)
  string(REGEX MATCH "define[ \t]+RD_KAFKA_VERSION[ \t]+0x+([a-fA-F0-9]+)" _LibRDKafka_version_match
               "${_LibRDKafka_version_header}"
  )
  set(LIBRDKAKFA_VERSION_HEX "${CMAKE_MATCH_1}")
  hex2decversion(LibRDKafka_VERSION ${LIBRDKAKFA_VERSION_HEX})

  if(${LibRDKafka_VERSION} VERSION_LESS ${LibRDKafka_FIND_VERSION})
    set(LibRDKafka_VERSION_OK FALSE)
  else(${LibRDKafka_VERSION} VERSION_LESS ${LibRDKafka_FIND_VERSION})
    set(LibRDKafka_VERSION_OK TRUE)
  endif(${LibRDKafka_VERSION} VERSION_LESS ${LibRDKafka_FIND_VERSION})

  if(NOT LibRDKafka_VERSION_OK)
    message(SEND_ERROR "LibRDKafka version ${LibRDKafka_VERSION} found in ${LibRDKafka_INCLUDE_DIR}, "
                       "but at least version ${LibRDKafka_FIND_VERSION} is required"
    )
  endif(NOT LibRDKafka_VERSION_OK)
endmacro(_LibRDKafka_check_version)

find_path(
  LibRDKafka_ROOT_DIR
  NAMES include/librdkafka/rdkafkacpp.h
  PATHS /usr/local
)
find_path(
  LibRDKafka_INCLUDE_DIR
  NAMES librdkafka/rdkafkacpp.h
  HINTS ${LibRDKafka_ROOT_DIR}/include
)
find_library(
  LibRDKafka
  NAMES rdkafka++ librdkafkacpp
  HINTS ${LibRDKafka_ROOT_DIR}/lib
)
find_library(
  LibRDKafka_DEBUG
  NAMES librdkafkacpp_D
  HINTS ${LibRDKafka_ROOT_DIR}/lib
)
find_library(
  LibRDKafka_C
  NAMES rdkafka librdkafka
  HINTS ${LibRDKafka_ROOT_DIR}/lib
)
find_library(
  LibRDKafka_C_DEBUG
  NAMES librdkafka_D
  HINTS ${LIBRDKafka_ROOT_DIR}/lib
)

if(LibRDKafka_INCLUDE_DIR)
  _librdkafka_check_version()
endif()

if(LibRDKafka_DEBUG)

  set(LibRDKafka_LIBRARIES optimized ${LibRDKafka} debug ${LibRDKafka_DEBUG})

else()

  set(LibRDKafka_LIBRARIES ${LibRDKafka})

endif()

if(LibRDKafka_C_DEBUG)

  set(LibRDKafka_C_LIBRARIES optimized ${LibRDKafka_C} debug ${LibRDKafka_C_DEBUG})

else()

  set(LibRDKafka_C_LIBRARIES ${LibRDKafka_C})

endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
  LibRDKafka DEFAULT_MSG LibRDKafka_LIBRARIES LibRDKafka_C_LIBRARIES LibRDKafka_INCLUDE_DIR
)

mark_as_advanced(LibRDKafka_ROOT_DIR LibRDKafka_LIBRARIES LibRDKafka_C_LIBRARIES LibRDKafka_INCLUDE_DIR)
