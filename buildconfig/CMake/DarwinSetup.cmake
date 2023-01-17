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

# Guess at Qt5 dir according to homebrew locations, unless already specified
if(NOT Qt5_DIR)
  message(STATUS "No Qt5 directory specified, guessing in default Homebrew locations")
  foreach(_qtdir /usr/local/opt/qt@5/lib/cmake/Qt5 /usr/local/opt/qt/lib/cmake/Qt5)
    if(EXISTS ${_qtdir})
      set(Qt5_DIR ${_qtdir})
      break()
    endif()
  endforeach()
endif()

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

# ######################################################################################################################
# Mac-specific installation setup
# ######################################################################################################################
# use homebrew OpenSSL package
if(NOT OpenSSL_ROOT)
  set(OpenSSL_ROOT /usr/local/opt/openssl)
endif()

if(NOT HDF5_ROOT)
  set(HDF5_ROOT /usr/local/opt/hdf5) # Only for homebrew!
endif()

if(ENABLE_WORKBENCH AND NOT CONDA_BUILD)
  set(CPACK_GENERATOR DragNDrop)
  set(CMAKE_INSTALL_PREFIX
      ""
      CACHE PATH ""
  )
  # Replace hdiutil command to retry on detach failure
  set(CPACK_COMMAND_HDIUTIL ${CMAKE_SOURCE_DIR}/installers/MacInstaller/hdiutilwrap)
  set(CMAKE_MACOSX_RPATH 1)
  set(CPACK_DMG_BACKGROUND_IMAGE ${CMAKE_SOURCE_DIR}/installers/conda/osx/dmg_background.png)

  set(WORKBENCH_BUNDLE MantidWorkbench.app/Contents/)
  set(WORKBENCH_APP MantidWorkbench${CPACK_PACKAGE_SUFFIX_CAMELCASE}.app)
  set(WORKBENCH_BUNDLE ${WORKBENCH_APP}/Contents/)
  set(WORKBENCH_BIN_DIR ${WORKBENCH_BUNDLE}MacOS)
  set(WORKBENCH_LIB_DIR ${WORKBENCH_BUNDLE}MacOS)
  set(WORKBENCH_SITE_PACKAGES ${WORKBENCH_BUNDLE}MacOS)
  set(WORKBENCH_PLUGINS_DIR ${WORKBENCH_BUNDLE}PlugIns)

  install(
    FILES ${CMAKE_SOURCE_DIR}/images/mantid_workbench${CPACK_PACKAGE_SUFFIX}.icns
    DESTINATION ${WORKBENCH_BUNDLE}Resources/
    COMPONENT Runtime
  )
  set(BUNDLES ${INBUNDLE} ${WORKBENCH_BUNDLE})

  # Produce script to move icons in finder window to the correct locations
  configure_file(
    ${CMAKE_SOURCE_DIR}/installers/MacInstaller/CMakeDMGSetup.scpt.in ${CMAKE_BINARY_DIR}/DMGSetup.scpt @ONLY
  )
  set(CPACK_DMG_DS_STORE_SETUP_SCRIPT ${CMAKE_BINARY_DIR}/DMGSetup.scpt)
endif()
