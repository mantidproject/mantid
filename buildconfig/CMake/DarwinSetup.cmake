# ######################################################################################################################
# Set installation variables
# ######################################################################################################################
set(BIN_DIR bin)
set(ETC_DIR etc)
set(LIB_DIR lib)
set(SITE_PACKAGES ${LIB_DIR})
set(PLUGINS_DIR plugins)

set(WORKBENCH_BIN_DIR ${BIN_DIR})
set(WORKBENCH_LIB_DIR ${LIB_DIR})
set(WORKBENCH_SITE_PACKAGES
    ${LIB_DIR}
    CACHE PATH "Location of site packages"
)
set(WORKBENCH_PLUGINS_DIR ${PLUGINS_DIR})

# Determine the version of macOS that we are running
# ######################################################################################################################

# Set the system name (and remove the space)
execute_process(
  COMMAND /usr/bin/sw_vers -productVersion
  OUTPUT_VARIABLE MACOS_VERSION
  RESULT_VARIABLE MACOS_VERSION_STATUS
)
# Strip off any /CR or /LF
string(STRIP ${MACOS_VERSION} MACOS_VERSION)

if(MACOS_VERSION VERSION_LESS 10.13)
  message(FATAL_ERROR "The minimum supported version of Mac OS X is 10.13 (High Sierra).")
endif()

# Export variables globally
set(MACOS_VERSION
    ${MACOS_VERSION}
    CACHE INTERNAL ""
)

message(STATUS "Operating System: macOS ${MACOS_VERSION}")

# Enable the use of the -isystem flag to mark headers in Third_Party as system headers
set(CMAKE_INCLUDE_SYSTEM_FLAG_CXX "-isystem ")

# Python flags
set(PY_VER "${Python_VERSION_MAJOR}.${Python_VERSION_MINOR}")
execute_process(
  COMMAND python${PY_VER}-config --prefix
  OUTPUT_VARIABLE PYTHON_PREFIX
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

execute_process(
  COMMAND python${PY_VER}-config --abiflags
  OUTPUT_VARIABLE PY_ABI
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Tag used by dynamic loader to identify directory of loading library
set(DL_ORIGIN_TAG @loader_path)

# directives similar to linux for conda framework-only build
set(BIN_DIR bin)
set(WORKBENCH_BIN_DIR bin)
set(ETC_DIR etc)
set(LIB_DIR lib)
set(SITE_PACKAGES lib)
set(PLUGINS_DIR plugins)
