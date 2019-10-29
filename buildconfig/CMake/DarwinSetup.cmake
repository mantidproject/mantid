# ##############################################################################
# Determine the version of macOS that we are running
# ##############################################################################

# Set the system name (and remove the space)
execute_process(
  COMMAND
    /usr/bin/sw_vers
    -productVersion
  OUTPUT_VARIABLE MACOS_VERSION
  RESULT_VARIABLE MACOS_VERSION_STATUS
)
# Strip off any /CR or /LF
string(
  STRIP
    ${MACOS_VERSION}
    MACOS_VERSION
)

if(MACOS_VERSION VERSION_LESS 10.13)
  message(
    FATAL_ERROR
      "The minimum supported version of Mac OS X is 10.13 (High Sierra)."
  )
endif()

if(MACOS_VERSION VERSION_GREATER 10.13 OR MACOS_VERSION VERSION_EQUAL 10.13)
  set(MACOS_CODENAME "High Sierra")
elseif(MACOS_VERSION VERSION_GREATER 10.14 OR MACOS_VERSION VERSION_EQUAL 10.14)
  set(MACOS_CODENAME "Mojave")
else()
  message(FATAL_ERROR "Unknown version of macOS")
endif()

# Export variables globally
set(
  MACOS_VERSION
  ${MACOS_VERSION}
  CACHE
    INTERNAL
    ""
)
set(
  MACOS_CODENAME
  ${MACOS_CODENAME}
  CACHE
    INTERNAL
    ""
)

message(
  STATUS "Operating System: Mac OS X ${MACOS_VERSION} (${MACOS_CODENAME})"
)

# Enable the use of the -isystem flag to mark headers in Third_Party as system
# headers
set(CMAKE_INCLUDE_SYSTEM_FLAG_CXX "-isystem ")

# Set Qt5 dir according to homebrew location
set(Qt5_DIR /usr/local/opt/qt/lib/cmake/Qt5)

# ##############################################################################
# Use python libraries associated with PYTHON_EXECUTABLE If unspecified, use
# first python executable in the PATH.
# ##############################################################################

# Find the python interpreter to get the version we're using (needed for install
# commands below)
find_package(PythonInterp)
if(PYTHON_VERSION_MAJOR)
  set(PY_VER "${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR}")
  message(
    STATUS
      "Python version is "
      ${PY_VER}
  )
else()
  # Older versions of CMake don't set these variables so just assume 2.7
  set(PY_VER 2.7)
endif()

execute_process(
  COMMAND
    python${PY_VER}-config
    --prefix
  OUTPUT_VARIABLE PYTHON_PREFIX
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

if(${PYTHON_VERSION_MAJOR} GREATER 2)
  execute_process(
    COMMAND
      python${PY_VER}-config
      --abiflags
    OUTPUT_VARIABLE PY_ABI
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
else()
  # --abiflags option not available in python 2
  set(PY_ABI "")
endif()

set(
  PYTHON_LIBRARY
  "${PYTHON_PREFIX}/lib/libpython${PY_VER}${PY_ABI}.dylib"
  CACHE
    FILEPATH
    "PYTHON_LIBRARY"
    FORCE
)
set(
  PYTHON_INCLUDE_DIR
  "${PYTHON_PREFIX}/include/python${PY_VER}${PY_ABI}"
  CACHE
    PATH
    "PYTHON_INCLUDE_DIR"
    FORCE
)

find_package(
  PythonLibs
  REQUIRED
)
# If found, need to add debug library into libraries variable
if(PYTHON_DEBUG_LIBRARIES)
  set(
    PYTHON_LIBRARIES
    optimized
    ${PYTHON_LIBRARIES}
    debug
    ${PYTHON_DEBUG_LIBRARIES}
  )
endif()

# Generate a target to put a mantidpython wrapper in the appropriate directory
if(NOT TARGET mantidpython)
  if(MAKE_VATES)
    set(
      PARAVIEW_PYTHON_PATHS
      ":${ParaView_DIR}/lib:${ParaView_DIR}/lib/site-packages:${ParaView_DIR}/lib/site-packages/vtk"
    )
  else()
    set(PARAVIEW_PYTHON_PATHS "")
  endif()
  configure_file(
    ${CMAKE_MODULE_PATH}/Packaging/osx/mantidpython.in
    ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/mantidpython
    @ONLY
  )

  add_custom_target(
    mantidpython
    ALL
    COMMAND
      ${CMAKE_COMMAND}
      -E
      copy_if_different
      ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/mantidpython
      ${PROJECT_BINARY_DIR}/bin/${CMAKE_CFG_INTDIR}/mantidpython
    COMMENT "Generating mantidpython"
  )
  # Configure install script at the same time. Doing it later causes a warning
  # from ninja.
  if(MAKE_VATES)
    set(
      PARAVIEW_PYTHON_PATHS
      ":\${SCRIPT_PATH}/../Libraries:\${SCRIPT_PATH}/../Python:\${SCRIPT_PATH}/../Python/vtk"
    )
  else()
    set(PARAVIEW_PYTHON_PATHS "")
  endif()

  set(PYTHONHOME "\${INSTALLDIR}/Frameworks/Python.framework/Versions/2.7")
  configure_file(
    ${CMAKE_MODULE_PATH}/Packaging/osx/mantidpython.in
    ${CMAKE_BINARY_DIR}/mantidpython_osx_install
    @ONLY
  )
  unset(PYTHONHOME)
endif()

# directives similar to linux for conda framework-only build
set(BIN_DIR bin)
set(WORKBENCH_BIN_DIR bin)
set(ETC_DIR etc)
set(LIB_DIR lib)
set(PLUGINS_DIR plugins)

# ##############################################################################
# Mac-specific installation setup
# ##############################################################################
# use homebrew OpenSSL package
if(NOT OPENSSL_ROOT_DIR)
  set(OPENSSL_ROOT_DIR /usr/local/opt/openssl)
endif(NOT OPENSSL_ROOT_DIR)

if(NOT HDF5_ROOT)
  set(HDF5_ROOT /usr/local/opt/hdf5)
endif()

if(ENABLE_MANTIDPLOT OR ENABLE_WORKBENCH)
  set(CPACK_GENERATOR DragNDrop)
  set(CMAKE_INSTALL_PREFIX "")
  set(CPACK_PACKAGE_EXECUTABLES MantidPlot)
  set(CMAKE_MACOSX_RPATH 1)
  set(
    CPACK_DMG_BACKGROUND_IMAGE
    ${CMAKE_SOURCE_DIR}/images/osx-bundle-background.png
  )
  set(
    CPACK_DMG_DS_STORE_SETUP_SCRIPT
    ${CMAKE_SOURCE_DIR}/installers/MacInstaller/CMakeDMGSetup.scpt
  )
  set(MACOSX_BUNDLE_ICON_FILE MantidPlot.icns)
  string(
    REPLACE
      " "
      ""
      CPACK_SYSTEM_NAME
      ${MACOS_CODENAME}
  )

  if(ENABLE_MANTIDPLOT)
    set(INBUNDLE MantidPlot.app/Contents/)
    # Copy the launcher script to the correct location
    configure_file(
      ${CMAKE_MODULE_PATH}/Packaging/osx/Mantid_osx_launcher.in
      ${CMAKE_BINARY_DIR}/bin/MantidPlot.app/Contents/MacOS/Mantid_osx_launcher
      @ONLY
    )

    # We know exactly where this has to be on Darwin, but separate whether we
    # have kit build or a regular build.
    if(ENABLE_CPACK AND MAKE_VATES)
      add_definitions(-DBUNDLE_PARAVIEW)
    else()
      set(PARAVIEW_APP_DIR "${ParaView_DIR}")
      set(PARAVIEW_APP_BIN_DIR "${PARAVIEW_APP_DIR}/bin")
      set(PARAVIEW_APP_LIB_DIR "${PARAVIEW_APP_DIR}/lib")
      set(PARAVIEW_APP_PLUGIN_DIR "${PARAVIEW_APP_DIR}/lib")
    endif()

    set(BIN_DIR ${INBUNDLE}/MacOS)
    set(LIB_DIR ${INBUNDLE}/MacOS)
    # This is the root of the plugins directory
    set(PLUGINS_DIR ${INBUNDLE}PlugIns)
    # Separate directory of plugins to be discovered by the ParaView framework
    # These cannot be mixed with our other plugins. Further sub-directories
    # based on the Qt version will also be created by the installation targets
    set(PVPLUGINS_SUBDIR paraview)

    install(
      FILES ${CMAKE_SOURCE_DIR}/images/MantidPlot.icns
      DESTINATION MantidPlot.app/Contents/Resources/
    )
    # Add launcher script for mantid python
    install(
      PROGRAMS ${CMAKE_BINARY_DIR}/mantidpython_osx_install
      DESTINATION MantidPlot.app/Contents/MacOS/
      RENAME mantidpython
    )
    # Add launcher application for a Mantid IPython console
    install(
      PROGRAMS ${CMAKE_MODULE_PATH}/Packaging/osx/MantidPython_osx_launcher
      DESTINATION MantidPython\ \(optional\).app/Contents/MacOS/
      RENAME MantidPython
    )
    install(
      FILES ${CMAKE_MODULE_PATH}/Packaging/osx/mantidpython_Info.plist
      DESTINATION MantidPython\ \(optional\).app/Contents/
      RENAME Info.plist
    )
    install(
      FILES ${CMAKE_SOURCE_DIR}/images/MantidPython.icns
      DESTINATION MantidPython\ \(optional\).app/Contents/Resources/
    )
    # Add launcher application for Mantid IPython notebooks
    install(
      PROGRAMS ${CMAKE_MODULE_PATH}/Packaging/osx/MantidNotebook_osx_launcher
      DESTINATION MantidNotebook\ \(optional\).app/Contents/MacOS/
      RENAME MantidNotebook
    )
    install(
      FILES ${CMAKE_MODULE_PATH}/Packaging/osx/mantidnotebook_Info.plist
      DESTINATION MantidNotebook\ \(optional\).app/Contents/
      RENAME Info.plist
    )
    install(
      FILES ${CMAKE_SOURCE_DIR}/images/MantidNotebook.icns
      DESTINATION MantidNotebook\ \(optional\).app/Contents/Resources/
    )
  endif()

  if(ENABLE_WORKBENCH)
    set(WORKBENCH_BUNDLE MantidWorkbench.app/Contents/)
    set(WORKBENCH_BIN_DIR ${WORKBENCH_BUNDLE}MacOS)
    set(WORKBENCH_LIB_DIR ${WORKBENCH_BUNDLE}MacOS)
    set(WORKBENCH_PLUGINS_DIR ${WORKBENCH_BUNDLE}PlugIns)

    # Add launcher application for a Mantid Workbench
    configure_file(
      ${CMAKE_MODULE_PATH}/Packaging/osx/MantidWorkbench_osx_launcher.in
      ${CMAKE_BINARY_DIR}/MantidWorkbench_osx_launcher.install
      @ONLY
    )

    install(
      PROGRAMS ${CMAKE_BINARY_DIR}/MantidWorkbench_osx_launcher.install
      DESTINATION MantidWorkbench.app/Contents/MacOS/
      RENAME MantidWorkbench
    )
    install(
      FILES ${CMAKE_MODULE_PATH}/Packaging/osx/mantidworkbench_Info.plist
      DESTINATION MantidWorkbench.app/Contents/
      RENAME Info.plist
    )
    install(
      FILES ${CMAKE_SOURCE_DIR}/images/MantidWorkbench.icns
      DESTINATION MantidWorkbench.app/Contents/Resources/
      RENAME MantidWorkbench.icns
    )
  endif()
  set(
    BUNDLES
    ${INBUNDLE}
    ${WORKBENCH_BUNDLE}
  )
endif()
