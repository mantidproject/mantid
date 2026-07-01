# CMake version check.
cmake_minimum_required(VERSION 3.21)

# Define the project name.
project(Mantid)

# Use modern >=2.8.12 INTERFACE_LINK_LIBRARIES behaviour: https://cmake.org/cmake/help/latest/policy/CMP0022.html
if(POLICY CMP0022)
  cmake_policy(SET CMP0022 NEW)
endif()
# find_package() uses <PackageName>_ROOT variables: https://cmake.org/cmake/help/latest/policy/CMP0074.html
if(POLICY CMP0074)
  cmake_policy(SET CMP0074 NEW)
endif()
# install() DESTINATION paths are normalized. https://cmake.org/cmake/help/latest/policy/CMP0177.html
if(POLICY CMP0177)
  cmake_policy(SET CMP0177 NEW)
endif()

# Add the path to our custom 'find' modules
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/buildconfig/CMake")
# Send libraries to common place, reference in several places so must be at top level
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)

# Control which components of the project to build
option(ENABLE_MANTIDQT "Enable Qt-based components" ON)
option(ENABLE_WORKBENCH "Enable Qt-based gui" ON)

# Major version of Qt to build the GUI layer against (5 or 6). A single version is selected per build tree; the
# Qt-versioned target machinery in buildconfig/CMake/QtTargetFunctions.cmake produces the matching -Qt${V} targets.
# Linux defaults to Qt6; Windows and macOS still default to Qt5.
if(UNIX AND NOT APPLE)
  set(_default_qt_version 6)
else()
  set(_default_qt_version 5)
endif()
set(MANTID_QT_VERSION
    ${_default_qt_version}
    CACHE STRING "Major version of Qt to build against (5 or 6)"
)
set_property(CACHE MANTID_QT_VERSION PROPERTY STRINGS 5 6)

# Option to control whether the framework libraries are built or expected to be installed
set(MANTID_FRAMEWORK_LIB
    "BUILD"
    CACHE STRING "Mantid framework build options, valid options are (BUILD, SYSTEM).
If system is specified a valid mantid framework installation must be present on the system, e.g. from conda"
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
    OR ENABLE_DOCS
    OR MPI_BUILD)
)
  message(FATAL_ERROR "Must select at least one component to build")
endif()

# Create two internal BUILD_* variables. With these, we can do consistent checks throughout, e.g: if(BUILD_MANTIDQT OR
# ENABLE_WORKBENCH) .... endif() to check if we are building the mantidqt libs or the workbench application
if(MANTID_QT_LIB STREQUAL "BUILD")
  set(BUILD_MANTIDQT ON)
endif()
if(MANTID_FRAMEWORK_LIB STREQUAL "BUILD")
  set(BUILD_MANTIDFRAMEWORK ON)
endif()

# Conda build variable
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

# Bootstrap any dependencies
include(Bootstrap)

if(CMAKE_SYSTEM_NAME MATCHES "Windows")
  include(WindowsSetup)
elseif(CMAKE_SYSTEM_NAME MATCHES "Darwin")
  include(DarwinSetup)
elseif(CMAKE_SYSTEM_NAME MATCHES "Linux")
  include(LinuxSetup)
else()
  message(WARNING "Unknown platform detected: ${CMAKE_SYSTEM_NAME}")
endif()

# Turn this on to set compiler warnings as errors REMEMBER: unchanged files don't necessarily get recompiled so the
# issues might hide unless a clean build
option(CMAKE_COMPILE_WARNING_AS_ERROR "Compile warnings as errors" OFF)

# Cross-platform setup
include(CommonSetup)

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

set(QT_MIN_VERSION 5.15)
set(QT6_MIN_VERSION 6.11)

if(ENABLE_WORKBENCH OR BUILD_MANTIDQT)
  # Resolve the requested major version using Qt's own multi-version convention. This sets QT_VERSION_MAJOR, which the
  # rest of the GUI build keys off.
  if(MANTID_QT_VERSION EQUAL 6)
    set(_qt_min_version ${QT6_MIN_VERSION})
    find_package(
      QT ${QT6_MIN_VERSION} NAMES Qt6
      COMPONENTS Core Gui Widgets OpenGL Test
      REQUIRED
    )
  else()
    set(_qt_min_version ${QT_MIN_VERSION})
    find_package(
      QT ${QT_MIN_VERSION} NAMES Qt5
      COMPONENTS Core Gui Widgets OpenGL Test
      REQUIRED
    )
  endif()
  find_package(
    Qt${QT_VERSION_MAJOR} ${_qt_min_version}
    COMPONENTS Core Gui Widgets OpenGL Test
    REQUIRED
  )
  if(QT_VERSION_MAJOR EQUAL 6)
    # QOpenGLWidget moved out of the OpenGL module into its own module in Qt6
    find_package(
      Qt6
      COMPONENTS OpenGLWidgets
      REQUIRED
    )
  endif()
  message(STATUS "Found Qt ${Qt${QT_VERSION_MAJOR}_VERSION}: ${Qt${QT_VERSION_MAJOR}_DIR}")
  if(BUILD_MANTIDQT)
    find_package(QScintillaQt${QT_VERSION_MAJOR} REQUIRED)
  endif()
  find_package(PyQt${QT_VERSION_MAJOR} REQUIRED)
  find_package(SIP REQUIRED)
  if(QT_VERSION_MAJOR EQUAL 5)
    # Only the legacy sip<=v4 PyQt5 path consumes these flags
    separate_arguments(PYQT5_SIP_FLAGS)
  endif()
  # Disable use of C++ API deprecated before the given version. Use the same 5.15 baseline for both builds: under Qt6
  # this keeps APIs deprecated in 6.x (e.g. QMouseEvent::pos(), QVariant::Type) available, so the migration can defer
  # cleaning those up - only APIs actually *removed* in Qt6 must be fixed.
  add_compile_definitions(QT_DISABLE_DEPRECATED_BEFORE=0x050F00)
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
  if(MPI_BUILD)
    find_package(MantidFramework REQUIRED)
    add_subdirectory(Framework/MPIAlgorithms)
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
    COMMAND ${Python_EXECUTABLE} -c "import mantid_sphinx_theme"
    OUTPUT_VARIABLE MANTID_SPHINX_THEME_OUT
    ERROR_VARIABLE MANTID_SPHINX_THEME_ERR
    OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_STRIP_TRAILING_WHITESPACE
  )
  if(MANTID_SPHINX_THEME_ERR)
    message(ERROR " Did not find mantid_sphinx_theme")
    message(STATUS "${Python_EXECUTABLE} -c \"import mantid_sphinx_theme\"")
    message(STATUS "${MANTID_SPHINX_THEME_ERR}")
    message(FATAL_ERROR " Install instructions at https://pypi.python.org/pypi/mantid-sphinx-theme/")
  endif()

  add_subdirectory(dev-docs)
  add_subdirectory(docs)
endif()

# System test data target
if(BUILD_MANTIDFRAMEWORK
   OR BUILD_MANTIDQT
   OR ENABLE_WORKBENCH
)
  add_subdirectory(Testing/SystemTests/scripts)
  add_subdirectory(Testing/SystemTests/tests)
endif()

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

# THIS MUST BE THE LAST SUB_DIRECTORY ADDED. See Framework/PostInstall/CMakeLists for an explanation
add_subdirectory(Framework/PostInstall)
