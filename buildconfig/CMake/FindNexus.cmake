# ######################################################################################################################
# * Attempt to find NeXus C/C++ libraries and include files NEXUS_INCLUDE_DIR where to find napi.h, etc.
#   NEXUS_C_LIBRARIES C libraries to link against NEXUS_CPP_LIBRARIES C++ libraries to link against NEXUS_LIBRARIES All
#   libraries that have been found NEXUS_VERSION The NeXus version NEXUS_FOUND If false, do not try to use NeXus
# ######################################################################################################################

# Search for the include files. Newer versions of Nexus have a nexus sub directory containing the headers but older
# versions just have napi.h in the root include path Try for nexus/napi.h first
find_path(NEXUS_INCLUDE_DIR napi.h PATH_SUFFIXES nexus)

# Find the C libraries
if(WIN32 AND CONDA_ENV)
  find_library(NEXUS_C_LIBRARIES NAMES NeXus NeXus)
  # Find the C++ libraries
  find_library(NEXUS_CPP_LIBRARIES NAMES NeXusCPP NeXusCPP)
else()
  find_library(NEXUS_C_LIBRARIES NAMES NeXus libNeXus-0)
  # Find the C++ libraries
  find_library(NEXUS_CPP_LIBRARIES NAMES NeXusCPP libNeXusCPP-0)
endif()

# Debug variants C
find_library(NEXUS_C_LIBRARIES_DEBUG NAMES NeXus libNeXus-0D)
# C++
find_library(NEXUS_CPP_LIBRARIES_DEBUG NAMES NeXusCPP libNeXusCPP-0D)

if(NEXUS_C_LIBRARIES_DEBUG AND NEXUS_CPP_LIBRARIES_DEBUG)
  set(NEXUS_LIBRARIES
      optimized
      ${NEXUS_C_LIBRARIES}
      optimized
      ${NEXUS_CPP_LIBRARIES}
      debug
      ${NEXUS_C_LIBRARIES_DEBUG}
      debug
      ${NEXUS_CPP_LIBRARIES_DEBUG}
  )
else()
  set(NEXUS_LIBRARIES ${NEXUS_C_LIBRARIES} ${NEXUS_CPP_LIBRARIES})
endif()

# Set a version string by examining the napi.h header
if(NEXUS_INCLUDE_DIR)
  # Extract the line containing the version string which will look like this "#define NEXUS_VERSION   "X.X.X" /*
  # major.minor.patch */"
  file(STRINGS ${NEXUS_INCLUDE_DIR}/napi.h NEXUS_VERSION_TMP
       REGEX "^#define[ \t]+NEXUS_VERSION[ \t]+\"[0-9]+.[0-9]+.[0-9]+\"[ \t]+/\\* major\\.minor\\.patch \\*/$"
  )
  # Hack off the portion up to and including the first double quote
  string(REGEX REPLACE "^#define[ \t]+NEXUS_VERSION[ \t]+\"" "" NEXUS_VERSION_TMP ${NEXUS_VERSION_TMP})
  # Hack off the portion from the second double quote to the end of the line
  string(REGEX REPLACE "\"[ \t]+/\\* major\\.minor\\.patch \\*/$" "" NEXUS_VERSION_TMP ${NEXUS_VERSION_TMP})
  set(NEXUS_VERSION
      ${NEXUS_VERSION_TMP}
      CACHE STRING "" FORCE
  )
endif()

# Handle the QUIETLY and REQUIRED arguments and set NEXUS_FOUND to TRUE if all listed variables are TRUE
include(FindPackageHandleStandardArgs)
if(NEXUS_VERSION)
  message(STATUS "NeXus version ${NEXUS_VERSION}")
  find_package_handle_standard_args(
    Nexus
    REQUIRED_VARS NEXUS_LIBRARIES NEXUS_INCLUDE_DIR
    VERSION_VAR NEXUS_VERSION
  )
  # this and related code should go away when libraries are upgraded
  if(NEXUS_VERSION VERSION_LESS 4.4)
    add_definitions(-DNEXUS43)
  endif()
else(NEXUS_VERSION)
  message(WARNING "Failed to determine version: Ignoring version requirement")
  find_package_handle_standard_args(Nexus DEFAULT_MSG NEXUS_LIBRARIES NEXUS_INCLUDE_DIR)
endif(NEXUS_VERSION)

if(WIN32)
  # c library
  string(REPLACE ".lib" ".dll" NEXUS_C_LIBRARIES_DLL "${NEXUS_C_LIBRARIES}")
  string(REPLACE ".lib" ".dll" NEXUS_C_LIBRARIES_DEBUG_DLL "${NEXUS_C_LIBRARIES_DEBUG}")
  get_filename_component(NEXUS_C_LIBRARIES_DLL ${NEXUS_C_LIBRARIES_DLL} NAME)
  get_filename_component(NEXUS_C_LIBRARIES_DEBUG_DLL ${NEXUS_C_LIBRARIES_DEBUG_DLL} NAME)
  find_file(NEXUS_C_LIBRARIES_DLL PATH_SUFFIXES bin/)
  find_file(NEXUS_C_LIBRARIES_DEBUG_DLL PATH_SUFFIXES bin/)

  # cpp library
  string(REPLACE ".lib" ".dll" NEXUS_CPP_LIBRARIES_DLL "${NEXUS_CPP_LIBRARIES}")
  string(REPLACE ".lib" ".dll" NEXUS_CPP_LIBRARIES_DEBUG_DLL "${NEXUS_CPP_LIBRARIES_DEBUG}")
  get_filename_component(NEXUS_CPP_LIBRARIES_DLL ${NEXUS_CPP_LIBRARIES_DLL} NAME)
  get_filename_component(NEXUS_CPP_LIBRARIES_DEBUG_DLL ${NEXUS_CPP_LIBRARIES_DEBUG_DLL} NAME)
  find_file(NEXUS_CPP_LIBRARIES_DLL PATH_SUFFIXES bin/)
  find_file(NEXUS_CPP_LIBRARIES_DEBUG_DLL PATH_SUFFIXES bin/)

endif()

if(NEXUS_FOUND
   AND NOT TARGET Nexus::nexus
   AND NOT TARGET Nexus::nexuscpp
)
  if(NEXUS_C_LIBRARIES_DLL AND NEXUS_CPP_LIBRARIES_DLL)

    # Nexus c library Windows systems with dll libraries.
    add_library(Nexus::nexus SHARED IMPORTED)

    # Windows with dlls, but only Release libraries.
    set_target_properties(
      Nexus::nexus
      PROPERTIES IMPORTED_LOCATION_RELEASE "${NEXUS_C_LIBRARIES_DLL}"
                 IMPORTED_IMPLIB "${NEXUS_C_LIBRARIES}"
                 INTERFACE_INCLUDE_DIRECTORIES "${NEXUS_INCLUDE_DIR}"
                 IMPORTED_CONFIGURATIONS Release
                 IMPORTED_LINK_INTERFACE_LANGUAGES "C"
    )

    # If we have both Debug and Release libraries
    if(NEXUS_C_LIBRARIES_DEBUG_DLL)
      set_property(
        TARGET Nexus::nexus
        APPEND
        PROPERTY IMPORTED_CONFIGURATIONS Debug
      )
      set_target_properties(
        Nexus::nexus PROPERTIES IMPORTED_LOCATION_DEBUG "${NEXUS_C_LIBRARIES_DEBUG_DLL}" IMPORTED_IMPLIB_DEBUG
                                                                                         "${NEXUS_C_LIBRARIES_DEBUG}"
      )
    endif()

    # Nexus cpp library Windows systems with dll libraries.
    add_library(Nexus::nexuscpp SHARED IMPORTED)

    # Windows with dlls, but only Release libraries.
    set_target_properties(
      Nexus::nexuscpp
      PROPERTIES IMPORTED_LOCATION_RELEASE "${NEXUS_CPP_LIBRARIES_DLL}"
                 IMPORTED_IMPLIB "${NEXUS_CPP_LIBRARIES}"
                 INTERFACE_INCLUDE_DIRECTORIES "${NEXUS_INCLUDE_DIR}"
                 IMPORTED_CONFIGURATIONS Release
                 IMPORTED_LINK_INTERFACE_LANGUAGES "CXX"
    )

    # If we have both Debug and Release libraries
    if(NEXUS_CPP_LIBRARIES_DEBUG_DLL)
      set_property(
        TARGET Nexus::nexuscpp
        APPEND
        PROPERTY IMPORTED_CONFIGURATIONS Debug
      )
      set_target_properties(
        Nexus::nexuscpp PROPERTIES IMPORTED_LOCATION_DEBUG "${NEXUS_CPP_LIBRARIES_DEBUG_DLL}"
                                   IMPORTED_IMPLIB_DEBUG "${NEXUS_CPP_LIBRARIES_DEBUG}"
      )
    endif()
  else()
    add_library(Nexus::nexus UNKNOWN IMPORTED)
    set_target_properties(
      Nexus::nexus
      PROPERTIES IMPORTED_LOCATION "${NEXUS_C_LIBRARIES}"
                 INTERFACE_INCLUDE_DIRECTORIES "${NEXUS_INCLUDE_DIR}"
                 IMPORTED_LINK_INTERFACE_LANGUAGES "C"
    )

    add_library(Nexus::nexuscpp UNKNOWN IMPORTED)
    set_target_properties(
      Nexus::nexuscpp
      PROPERTIES IMPORTED_LOCATION "${NEXUS_CPP_LIBRARIES}"
                 INTERFACE_INCLUDE_DIRECTORIES "${NEXUS_INCLUDE_DIR}"
                 IMPORTED_LINK_INTERFACE_LANGUAGES "CXX"
    )
  endif()
endif()
