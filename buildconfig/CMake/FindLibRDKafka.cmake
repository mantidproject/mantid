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

find_library(LibRDKafka_LIBRARIES
        NAMES rdkafka++
        HINTS ${LibRDKafka_ROOT_DIR}/lib
        )

find_library(LibRDKafka_C_LIBRARIES
        NAMES rdkafka
        HINTS ${LibRDKafka_ROOT_DIR}/lib
        )

find_path(LibRDKafka_INCLUDE_DIR
        NAMES librdkafka/rdkafkacpp.h
        HINTS ${LibRDKafka_ROOT_DIR}/include
        )

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
