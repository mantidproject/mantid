# CMake version check.
cmake_minimum_required(VERSION 3.16)

# Define the project name.
project(Mantid)

# Policy settings
if(POLICY CMP0022)
  cmake_policy(SET CMP0022 NEW)
endif()

if(POLICY CMP0074)
  cmake_policy(SET CMP0074 NEW)
endif()

# System package target is important for the windows builds as it allows us to package only the dlls and exes and
# exclude libs. Defaults to empty for other platforms.
set(SYSTEM_PACKAGE_TARGET "")

# Add the path to our custom 'find' modules
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/buildconfig/CMake")
# Send libraries to common place, reference in several places so must be at top level
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)

option(ENABLE_MANTIDQT "Enable Qt5-based components" ON)
option(ENABLE_WORKBENCH "Enable Qt5-based gui" ON)

set(MANTID_FRAMEWORK_LIB
    "BUILD"
    CACHE STRING "Mantid framework build options, valid options are (BUILD, SYSTEM).
If system is specificed a valid mantid framework installation must be present on the system, e.g from conda"
)
set_property(CACHE MANTID_FRAMEWORK_LIB PROPERTY STRINGS "BUILD" "SYSTEM")

# set this for compatability, as previously if enable_workbench was off we didn't build the qt element we can override
# this behavior on the command line.
if(NOT ENABLE_WORKBENCH)
  set(_mantidqt_option "OFF")
else()
  set(_mantidqt_option "BUILD")
endif()

set(MANTID_QT_LIB
    ${_mantidqt_option}
    CACHE STRING "Mantid qt build options, valid options are (OFF, BUILD, SYSTEM).
If system is specificed a valid mantid qt installation must be present on the system, e.g. from conda"
)
set_property(CACHE MANTID_QT_LIB PROPERTY STRINGS "OFF" "BUILD" "SYSTEM")

if(NOT
   (ENABLE_WORKBENCH
    OR MANTID_QT_LIB STREQUAL "BUILD"
    OR MANTID_FRAMEWORK_LIB STREQUAL "BUILD"
    OR ENABLE_DOCS)
)
  message(FATAL_ERROR "Must select atleast one component to build")
endif()

# Create two internal BUILD_* variables. With these, we can do consistent checks throughout, e.g: if(BUILD_MANTIDQT OR
# ENABLE_WORKBENCH) .... endif() to check if we are building the mantidqt libs or the workbench application
if(MANTID_QT_LIB STREQUAL "BUILD")
  set(BUILD_MANTIDQT ON)
endif()
if(MANTID_FRAMEWORK_LIB STREQUAL "BUILD")
  set(BUILD_MANTIDFRAMEWORK ON)
endif()

set(CPACK_INSTALL_CMAKE_PROJECTS "${CMAKE_BINARY_DIR}" "Mantid" "ALL" "/")

# Conda build variables
set(CONDA_ENV
    FALSE
    CACHE BOOL "If True then we are building in a conda environment.
          This predominately changes the operation of some the finders FindXXX.cmake
          On windows it also prevents the use of the 3rdparty dependency directory"
)
set(CONDA_BUILD
    FALSE
    CACHE
      BOOL
      "If True then we are using conda-build to build and package mantid.
          This stops a few things from occurring:
          - A bundle is no longer generated on OSX
          - Linux custom handling on the install is omitted
          - Prevents site-package manipulation by the installer"
)

# Ensure that if we are running any sanitizers the compiler options are known in sub targets
include(Sanitizers)

# Quick exit if we only want data targets
if(DATA_TARGETS_ONLY)
  include(SetupDataTargets)
  # System test config files
  add_subdirectory(Testing/SystemTests/scripts)
  return()
endif()

# Packaging configuration
set(CPACK_PACKAGE_SUFFIX
    "unstable"
    CACHE STRING "suffix used to determine the deployment type"
)
set_property(CACHE CPACK_PACKAGE_SUFFIX PROPERTY STRINGS nightly unstable "")
# empty string and release are treated as the same thing
if(CPACK_PACKAGE_SUFFIX)
  # Camelcase version of suffix for Windows/Mac
  string(TOUPPER ${CPACK_PACKAGE_SUFFIX} CPACK_PACKAGE_SUFFIX_CAMELCASE)
  string(SUBSTRING ${CPACK_PACKAGE_SUFFIX_CAMELCASE} 0 1 CPACK_PACKAGE_SUFFIX_CAMELCASE)
  string(SUBSTRING ${CPACK_PACKAGE_SUFFIX} 1 -1 _tmp_suffix_remainder)
  string(CONCAT CPACK_PACKAGE_SUFFIX_CAMELCASE ${CPACK_PACKAGE_SUFFIX_CAMELCASE} ${_tmp_suffix_remainder})
endif()

# Set package name here
set(CPACK_PACKAGE_NAME "mantid${CPACK_PACKAGE_SUFFIX}")

# Bootstrap any dependencies
include(Bootstrap)

if(WIN32)
  include(WindowsSetup)
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  include(DarwinSetup)
endif()

# Cross-platform setup
include(CommonSetup)

if(BUILD_MANTIDFRAMEWORK)
  include(Eigen)
endif()
include(Span)

# Find OpenGL
if(ENABLE_OPENGL AND (BUILD_MANTIDFRAMEWORK OR BUILD_MANTIDQT))
  # Use legacy libGL library
  set(OpenGL_GL_PREFERENCE LEGACY)
  find_package(OpenGL REQUIRED)
  if(NOT OPENGL_FOUND)
    message(FATAL_ERROR "OpenGL was not found.")
  endif()
endif()

# We probably don't want this to run on every build.
option(COVERAGE "Generate coverage data" OFF)

if(COVERAGE)
  include(Coverage)
  coverage_turn_on()
endif()

if(ENABLE_WORKBENCH OR BUILD_MANTIDQT)
  find_package(
    Qt5
    COMPONENTS Core Gui Widgets OpenGL Test
    REQUIRED
  )
  if(Qt5_FOUND)
    message(STATUS "Found Qt ${Qt5_VERSION}: ${Qt5_DIR}")
  endif()
  if(BUILD_MANTIDQT)
    find_package(QScintillaQt5 REQUIRED)
  endif()
  find_package(PyQt5 REQUIRED)
  find_package(SIP REQUIRED)
  separate_arguments(PYQT5_SIP_FLAGS)
endif()

# Now add in all the components

# Flag that this is a full build, i.e not framework only
set(FULL_PACKAGE_BUILD 1)
# Set the path to the built docs directory here so that it can be used in the Kernel project for populating the
# properties file
set(DOCS_BUILDDIR ${CMAKE_BINARY_DIR}/docs)

# Framework Build options
option(CXXTEST_ADD_PERFORMANCE "Switch to add performance tests to the list of tests run by ctest?")

include(TargetFunctions)

if(MANTID_FRAMEWORK_LIB STREQUAL "SYSTEM")
  if(BUILD_MANTIDQT)
    find_package(MantidFramework REQUIRED)
  endif()
else()
  add_subdirectory(Framework)
endif()

set(CORE_MANTIDLIBS
    Mantid::Kernel
    Mantid::HistogramData
    Mantid::Indexing
    Mantid::Beamline
    Mantid::Geometry
    Mantid::API
    Mantid::Types
)

if(UNIX)
  # Experimental feature. Unix only at this point.
  option(UNITY_BUILD "Switch for utilising unity builds. Faster builds for selected components.")
endif(UNIX)

add_custom_target(AllTests)

if(BUILD_MANTIDFRAMEWORK)
  add_dependencies(AllTests FrameworkTests)
endif()

if(BUILD_MANTIDQT OR ENABLE_WORKBENCH)
  add_custom_target(GUITests)
  add_dependencies(check GUITests)
  # Collect all tests together
  add_dependencies(AllTests GUITests)
  add_subdirectory(qt)
endif()

if(BUILD_MANTIDFRAMEWORK)
  add_subdirectory(scripts)
endif()

# Docs requirements
option(ENABLE_DOCS "Enable Building user and developer documentation" ON)
if(ENABLE_DOCS)
  find_package(Sphinx REQUIRED)
  # run python to see if the theme is installed
  execute_process(
    COMMAND ${Python_EXECUTABLE} -c "import sphinx_bootstrap_theme"
    OUTPUT_VARIABLE SPHINX_BOOTSTRAP_THEME_OUT
    ERROR_VARIABLE SPHINX_BOOTSTRAP_THEME_ERR
    OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_STRIP_TRAILING_WHITESPACE
  )
  if(SPHINX_BOOTSTRAP_THEME_ERR)
    message(ERROR " Did not find sphinx_bootstrap_theme")
    message(STATUS "${Python_EXECUTABLE} -c \"import sphinx_bootstrap_theme\"")
    message(STATUS "${SPHINX_BOOTSTRAP_THEME_ERR}")
    message(FATAL_ERROR " Install instructions at https://pypi.python.org/pypi/sphinx-bootstrap-theme/")
  endif()

  add_subdirectory(dev-docs)
  add_subdirectory(docs)
endif()

# System test data target
add_subdirectory(Testing/SystemTests/scripts)

if(COVERAGE)
  get_property(ALL_SRCS GLOBAL PROPERTY COVERAGE_SRCS)
  set(SRCS_FILE "")
  foreach(SRC ${ALL_SRCS})
    set(SRCS_FILE "${SRCS_FILE}\n${SRC}")
  endforeach()
  # remove initial \n
  string(SUBSTRING ${SRCS_FILE} 1 -1 SRCS_FILE)
  set(SRCS_FILENAME "${CMAKE_CURRENT_BINARY_DIR}/sources.txt")
  file(WRITE ${SRCS_FILENAME} ${SRCS_FILE})
  coverage_setup(${SRCS_FILENAME} OFF "${CMAKE_SOURCE_DIR}/buildconfig/CMake")
endif()

# Installation settings

# Install the files (.desktop and icon) to create a menu items
if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux" AND NOT CONDA_BUILD)
  function(uppercase_first_letter output_var input)
    string(LENGTH ${input} _inputlen)
    string(TOUPPER ${input} _allupper)
    string(SUBSTRING ${_allupper} 0 1 _first)
    string(SUBSTRING ${input} 1 ${_inputlen} _rest)
    set(${output_var}
        ${_first}${_rest}
        PARENT_SCOPE
    )
  endfunction()

  function(install_desktop_files filename app_name exec_cmd icon_file icon_install_file)
    set(DESKTOP_NAME ${app_name})
    set(DESKTOP_EXEC_CMD ${exec_cmd})
    get_filename_component(DESKTOP_ICON ${icon_install_file} NAME_WE)
    set(_output_file ${CMAKE_CURRENT_BINARY_DIR}/${filename}.install)
    configure_file(
      ${CMAKE_CURRENT_SOURCE_DIR}/installers/LinuxInstaller/launcher-template.desktop.in ${_output_file} @ONLY
    )
    install(
      FILES ${_output_file}
      DESTINATION /usr/share/applications
      RENAME ${filename}
    )
    install(
      FILES ${icon_file}
      DESTINATION /usr/share/pixmaps
      RENAME ${icon_install_file}
    )
  endfunction()

  set(_icon_suffix)
  if(CPACK_PACKAGE_SUFFIX)
    if(${CPACK_PACKAGE_SUFFIX} STREQUAL "nightly" OR ${CPACK_PACKAGE_SUFFIX} STREQUAL "unstable")
      set(_icon_suffix ${CPACK_PACKAGE_SUFFIX})
    endif()
  endif()

  set(IMAGES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/images)
  set(_app_name_suffix)
  if(CPACK_PACKAGE_SUFFIX)
    uppercase_first_letter(_app_name_suffix ${CPACK_PACKAGE_SUFFIX})
  endif()

  if(ENABLE_WORKBENCH)
    set(_icon_filename ${IMAGES_DIR}/mantid_workbench)
    if(_icon_suffix)
      set(_icon_filename ${_icon_filename}${_icon_suffix})
    endif()
    install_desktop_files(
      mantidworkbench${CPACK_PACKAGE_SUFFIX}.desktop "Mantid Workbench ${_app_name_suffix}"
      ${CMAKE_INSTALL_PREFIX}/bin/mantidworkbench ${_icon_filename}.png mantid_workbench${CPACK_PACKAGE_SUFFIX}.png
    )
  endif()
endif()

# THIS MUST BE THE LAST SUB_DIRECTORY ADDED. See Framework/PostInstall/CMakeLists for an explanation
add_subdirectory(Framework/PostInstall)

# Conda
option(ENABLE_CONDA "Switch to enable conda package generation")
if(ENABLE_CONDA)
  include(conda)
endif(ENABLE_CONDA)

# CPack settings

# Keep this in a defaulted-to-off option
option(ENABLE_CPACK "Switch to enable CPack package generation")
if(ENABLE_CPACK)
  include(CPackCommon)

  # Mac package settings
  set(CPACK_RESOURCE_FILE_LICENSE ${CMAKE_SOURCE_DIR}/installers/WinInstaller/License.txt)

  if(CMAKE_HOST_WIN32)
    include(WindowsNSIS)
  elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    include(CPackLinuxSetup)
  endif()

  # run cpack configuration
  include(CPack)
  # let people know what is coming out the other end
  message(STATUS "CPACK_PACKAGE_FILE_NAME = ${CPACK_PACKAGE_FILE_NAME}")
endif()
