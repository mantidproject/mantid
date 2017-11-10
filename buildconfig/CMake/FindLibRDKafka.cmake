# - Try to find LibRDKafka headers and libraries.
#
# Usage of this module as follows:
#
#     find_package(LibRDKafka)
#
# Variables used by this module, they can change the default behaviour and need
# to be set before calling find_package:
#
#  LibRDKafka_ROOT_DIR  Set this variable to the root installation of
#                    LibRDKafka if the module has problems finding
#                    the proper installation path.
#
# Variables defined by this module:
#
#  LIBRDKAFKA_FOUND              System has LibRDKafka libs/headers
#  LibRDKafka_LIBRARIES          The LibRDKafka libraries
#  LibRDKafka_INCLUDE_DIR        The location of LibRDKafka headers

find_path(LibRDKafka_ROOT_DIR
        NAMES include/librdkafka/rdkafkacpp.h
        PATHS /usr/local
        )   
find_path(LibRDKafka_INCLUDE_DIR
        NAMES librdkafka/rdkafkacpp.h
        HINTS ${LibRDKafka_ROOT_DIR}/include
        )
find_library(LibRDKafka
        NAMES rdkafka++ librdkafkacpp
        HINTS ${LibRDKafka_ROOT_DIR}/lib
        )
find_library(LibRDKafka_DEBUG
        NAMES librdkafkacpp_D
        HINTS ${LibRDKafka_ROOT_DIR}/lib
        )
find_library(LibRDKafka_C
        NAMES rdkafka librdkafka
        HINTS ${LibRDKafka_ROOT_DIR}/lib
        )
find_library(LibRDKafka_C_DEBUG
        NAMES librdkafka_D
        HINTS ${LIBRDKafka_ROOT_DIR}/lib
        )
        
if( LibRDKafka_DEBUG )

set( LibRDKafka_LIBRARIES optimized ${LibRDKafka}
                          debug ${LibRDKafka_DEBUG}
)

else ()

set( LibRDKafka_LIBRARIES ${LibRDKafka}
)

endif ()

if( LibRDKafka_C_DEBUG )

set( LibRDKafka_C_LIBRARIES optimized ${LibRDKafka_C}
                            debug ${LibRDKafka_C_DEBUG}
)

else ()

set( LibRDKafka_C_LIBRARIES ${LibRDKafka_C} 
)

endif ()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LibRDKafka DEFAULT_MSG
        LibRDKafka_LIBRARIES
        LibRDKafka_C_LIBRARIES
        LibRDKafka_INCLUDE_DIR
        )

mark_as_advanced(
        LibRDKafka_ROOT_DIR
        LibRDKafka_LIBRARIES
        LibRDKafka_C_LIBRARIES
        LibRDKafka_INCLUDE_DIR
)
