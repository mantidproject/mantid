# CMake version check.
cmake_minimum_required(VERSION 3.16)

# Define the project name.
project(Mantid)

# Policy settings
if(POLICY CMP0022)
  cmake_policy(SET CMP0022 NEW)
endif()

if(POLICY CMP0072)
  cmake_policy(SET CMP0072 OLD)
endif()

if(POLICY CMP0074)
  cmake_policy(SET CMP0074 NEW)
endif()

# System package target is important for the windows builds as it allows us to
# package only the dlls and exes and exclude libs. Defaults to empty for other
# platforms.
set(SYSTEM_PACKAGE_TARGET "")

# Add the path to our custom 'find' modules
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/buildconfig/CMake")
# Send libraries to common place, reference in several places so must be at top
# level
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)

option(ENABLE_MANTIDPLOT "Enable Qt4-based gui & components" OFF)
option(ENABLE_WORKBENCH "Enable Qt5-based gui & components" ON)
if(ENABLE_MANTIDPLOT)
  message( FATAL_ERROR "MantidPlot has been removed from the source code")
endif()

set(CPACK_INSTALL_CMAKE_PROJECTS "${CMAKE_BINARY_DIR}" "Mantid" "ALL" "/")

# Conda build variable
set(CONDA_BUILD FALSE CACHE BOOL "If True, proceed with a Conda build. Only relevant on Windows")

# Ensure that if we are running any sanitizers the compiler options are
# known in sub targets
include(Sanitizers)

# Quick exit if we only want data targets
if(DATA_TARGETS_ONLY)
  include(SetupDataTargets)
  # System test config files
  add_subdirectory(Testing/SystemTests/scripts)
  return()
endif()

# Packaging configuration
set(CPACK_PACKAGE_SUFFIX "unstable"
    CACHE STRING "suffix used to determine the deployment type")
set_property(CACHE CPACK_PACKAGE_SUFFIX
             PROPERTY STRINGS
                      nightly
                      unstable
                      "")
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

# Configure Eigen early
include(Eigen)
include(Span)

if( MSVC )
  include ( MSVCSetup )
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  include(DarwinSetup)
endif()

# Cross-platform setup
include(CommonSetup)

# Find OpenGL
if(ENABLE_OPENGL)
  find_package(OpenGL REQUIRED)
  # FindOpenGL does not (as of CMake 2.6.4) take notice of the REQUIRED
  if(NOT OPENGL_FOUND)
    message(FATAL_ERROR "OpenGL was not found.")
  endif()
endif()

find_package(TBB REQUIRED)

# We probably don't want this to run on every build.
option(COVERAGE "Generate coverage data" OFF)

if(COVERAGE)
  include(Coverage)
  coverage_turn_on()
endif()

if(ENABLE_WORKBENCH)
  find_package(Qt5
               COMPONENTS Core
                          Gui
                          Widgets
                          OpenGL
                          Test
               REQUIRED)
  if(Qt5_FOUND)
    message(STATUS "Found Qt ${Qt5_VERSION}: ${Qt5_DIR}")
  endif()
  find_package(QScintillaQt5 REQUIRED)
  find_package(PyQt5 REQUIRED)
  find_package(SIP REQUIRED)
  separate_arguments(PYQT5_SIP_FLAGS)
endif()

# Now add in all the components

# Flag that this is a full build, i.e not framework only
set(FULL_PACKAGE_BUILD 1)
# Set the path to the built docs directory here so that it can be used in the
# Kernel project for populating the properties file
set(DOCS_BUILDDIR ${CMAKE_BINARY_DIR}/docs)

# Framework Build options
option(CXXTEST_ADD_PERFORMANCE
       "Switch to add performance tests to the list of tests run by ctest?")

add_subdirectory(Framework)

include_directories(Framework/Kernel/inc)
include_directories(Framework/Json/inc)
include_directories(Framework/HistogramData/inc)
include_directories(Framework/Indexing/inc)
include_directories(Framework/Parallel/inc)
include_directories(Framework/Beamline/inc)
include_directories(Framework/Geometry/inc)
include_directories(Framework/API/inc)
include_directories(Framework/Types/inc)
include_directories(Framework/NexusGeometry/inc)

set(CORE_MANTIDLIBS Kernel HistogramData Indexing Beamline Geometry API Types)

if(UNIX)
  # Experimental feature. Unix only at this point.
  option(UNITY_BUILD
         "Switch for utilising unity builds. Faster builds for selected components.")
endif(UNIX)

if(MSVC)
  add_definitions(-DQWT_DLL)
endif()

add_custom_target(AllTests)
add_dependencies(AllTests FrameworkTests)

if(ENABLE_WORKBENCH)
  add_custom_target(GUITests)
  add_dependencies(check GUITests)
  # Collect all tests together
  add_dependencies(AllTests GUITests)
  add_subdirectory(qt)
endif()

add_subdirectory(scripts)

# Docs requirements
option(ENABLE_DOCS "Enable Building user and developer documentation" ON)
if (ENABLE_DOCS)
  find_package(Sphinx REQUIRED)
  # run python to see if the theme is installed
  execute_process(COMMAND ${Python_EXECUTABLE} -c
                          "import sphinx_bootstrap_theme"
                  OUTPUT_VARIABLE SPHINX_BOOTSTRAP_THEME_OUT
                  ERROR_VARIABLE SPHINX_BOOTSTRAP_THEME_ERR
                  OUTPUT_STRIP_TRAILING_WHITESPACE
                  ERROR_STRIP_TRAILING_WHITESPACE)
  if(SPHINX_BOOTSTRAP_THEME_ERR)
    message(ERROR " Did not find sphinx_bootstrap_theme")
    message(STATUS "${Python_EXECUTABLE} -c \"import sphinx_bootstrap_theme\"")
    message(STATUS "${SPHINX_BOOTSTRAP_THEME_ERR}")
    message(
      FATAL_ERROR
        " Install instructions at https://pypi.python.org/pypi/sphinx-bootstrap-theme/"
      )
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
  string(SUBSTRING ${SRCS_FILE}
                   1
                   -1
                   SRCS_FILE)
  set(SRCS_FILENAME "${CMAKE_CURRENT_BINARY_DIR}/sources.txt")
  file(WRITE ${SRCS_FILENAME} ${SRCS_FILE})
  coverage_setup(${SRCS_FILENAME} OFF "${CMAKE_SOURCE_DIR}/buildconfig/CMake")
endif()

# Installation settings

# Install the files (.desktop and icon) to create a menu items
if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  function(uppercase_first_letter output_var input)
  string(LENGTH ${input} _inputlen)
  string(TOUPPER ${input} _allupper)
  string(SUBSTRING ${_allupper} 0 1 _first)
  string(SUBSTRING ${input} 1 ${_inputlen} _rest)
  set(${output_var} ${_first}${_rest} PARENT_SCOPE)
  endfunction()

  function(install_desktop_files filename app_name exec_cmd icon_file icon_install_file)
    set(DESKTOP_NAME ${app_name})
    set(DESKTOP_EXEC_CMD ${exec_cmd})
    get_filename_component(DESKTOP_ICON ${icon_install_file} NAME_WE)
    set(_output_file ${CMAKE_CURRENT_BINARY_DIR}/${filename}.install)
    configure_file(
      ${CMAKE_CURRENT_SOURCE_DIR}/installers/LinuxInstaller/launcher-template.desktop.in
      ${_output_file} @ONLY)
    install(FILES ${_output_file}
            DESTINATION /usr/share/applications
            RENAME ${filename})
    install(FILES ${icon_file}
            DESTINATION /usr/share/pixmaps
            RENAME ${icon_install_file})
  endfunction()

  set(_icon_suffix)
  if(CPACK_PACKAGE_SUFFIX)
    if (${CPACK_PACKAGE_SUFFIX} STREQUAL "nightly" OR
        ${CPACK_PACKAGE_SUFFIX} STREQUAL "unstable")
      set(_icon_suffix ${CPACK_PACKAGE_SUFFIX})
    endif()
  endif()

  set( IMAGES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/images )
  set(_app_name_suffix)
  if(CPACK_PACKAGE_SUFFIX)
    uppercase_first_letter(_app_name_suffix ${CPACK_PACKAGE_SUFFIX})
  endif()

  if(ENABLE_WORKBENCH)
    set(_icon_filename ${IMAGES_DIR}/mantid_workbench)
    if(_icon_suffix)
      set(_icon_filename ${_icon_filename}${_icon_suffix})
    endif()
    install_desktop_files(mantidworkbench${CPACK_PACKAGE_SUFFIX}.desktop
                          "Mantid Workbench ${_app_name_suffix}"
                          ${CMAKE_INSTALL_PREFIX}/bin/mantidworkbench
                          ${_icon_filename}.png
                          mantid_workbench${CPACK_PACKAGE_SUFFIX}.png)
  endif()
endif()

# THIS MUST BE THE LAST SUB_DIRECTORY ADDED. See
# Framework/PostInstall/CMakeLists for an explanation
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
  set(CPACK_RESOURCE_FILE_LICENSE
      ${CMAKE_SOURCE_DIR}/installers/WinInstaller/License.txt)

  if(CMAKE_HOST_WIN32)
    include(WindowsNSIS)
  elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    include(CPackLinuxSetup)

    # rhel requirements - only used if package requested is rpm
    set(
      CPACK_RPM_PACKAGE_REQUIRES
      "boost169-date-time,"
      "boost169-regex,"
      "boost169-serialization,"
      "boost169-filesystem,"
      "boost169-system,"
      "nexus >= 4.3.1,gsl,glibc,muParser,tbb,librdkafka,"
      "OCE-draw,OCE-foundation,OCE-modeling,OCE-ocaf,OCE-visualization,"
      "poco-crypto,poco-data,poco-mysql,poco-sqlite,poco-odbc,poco-util,poco-xml,poco-zip,poco-net,poco-netssl,poco-foundation,"
      "hdf,hdf5,jsoncpp >= 0.7.0")
    if(ENABLE_WORKBENCH)
      set(CPACK_RPM_PACKAGE_REQUIRES
          "${CPACK_RPM_PACKAGE_REQUIRES},qt5-qtbase,qscintilla-qt5")
    endif()
    # python requirements
    set(
      CPACK_RPM_PACKAGE_REQUIRES
      "${CPACK_RPM_PACKAGE_REQUIRES},"
      "boost169-python3,"
      "python36-numpy,python36-scipy,python36-h5py,python36-PyCifRW,"
      "python36-six,python36-PyYAML,python36-requests,python36-toml,"
      "python36-ipython,python36-ipython-notebook"
    )
    if(ENABLE_WORKBENCH)
      set(CPACK_RPM_PACKAGE_REQUIRES
          "${CPACK_RPM_PACKAGE_REQUIRES},python36-qt5,python36-QtPy,"
          "python36-matplotlib-qt5,python36-psutil,python36-ipython-gui")
    endif()

    string(REPLACE ";"
                   ","
                   CPACK_RPM_PACKAGE_REQUIRES
                   ${CPACK_RPM_PACKAGE_REQUIRES}) # fix up the fact that it was
                                                  # made as an array
    string(REPLACE ",,"
                   ","
                   CPACK_RPM_PACKAGE_REQUIRES
                   ${CPACK_RPM_PACKAGE_REQUIRES})

    # ubuntu requirements - only used if package requested is deb
    set(
      CPACK_DEBIAN_PACKAGE_DEPENDS
      "libboost-date-time${Boost_MAJOR_VERSION}.${Boost_MINOR_VERSION}.${Boost_SUBMINOR_VERSION},"
      "libboost-regex${Boost_MAJOR_VERSION}.${Boost_MINOR_VERSION}.${Boost_SUBMINOR_VERSION},"
      "libboost-python${Boost_MAJOR_VERSION}.${Boost_MINOR_VERSION}.${Boost_SUBMINOR_VERSION},"
      "libboost-serialization${Boost_MAJOR_VERSION}.${Boost_MINOR_VERSION}.${Boost_SUBMINOR_VERSION},"
      "libboost-filesystem${Boost_MAJOR_VERSION}.${Boost_MINOR_VERSION}.${Boost_SUBMINOR_VERSION},"
      "libnexus0v5 (>= 4.3),"
      "libjsoncpp1 (>=0.7.0),"
      "libmuparser2v5,"
      "libtbb2,"
      "librdkafka1,librdkafka++1,"
      "libpocofoundation${POCO_SOLIB_VERSION},libpocoutil${POCO_SOLIB_VERSION},libpoconet${POCO_SOLIB_VERSION},libpoconetssl${POCO_SOLIB_VERSION},libpococrypto${POCO_SOLIB_VERSION},libpocoxml${POCO_SOLIB_VERSION}"
      )

    if("${UNIX_CODENAME}" STREQUAL "bionic")
      set(CPACK_DEBIAN_PACKAGE_DEPENDS
          "${CPACK_DEBIAN_PACKAGE_DEPENDS},libgsl23,"
          "liboce-foundation11,liboce-modeling11,"
          "libqscintilla2-qt4-13,"
          "jupyter-notebook,"
          "libhdf5-cpp-100")
      if(ENABLE_WORKBENCH)
        set(CPACK_DEBIAN_PACKAGE_DEPENDS
            "${CPACK_DEBIAN_PACKAGE_DEPENDS},libqscintilla2-qt5-13")
      endif()
    elseif ( "${UNIX_DIST}" MATCHES "Ubuntu" )
      message(WARNING "Unsupported Debian-like OS: ${UNIX_CODENAME}. Packaging is unlikely to work correctly.")
    endif()
    # python requirements
    set(CPACK_DEBIAN_PACKAGE_DEPENDS
        "${CPACK_DEBIAN_PACKAGE_DEPENDS},python3-h5py,"
        "python3-numpy,"
        "python3-requests,"
        "python3-sip,"
        "python3-six,"
        "python3-matplotlib,"
        "python3-qtpy,"
        "python3-scipy,"
        "python3-toml,"
        "python3-pycifrw (>= 4.2.1),"
        "python3-yaml,"
        "ipython3-qtconsole") # transitional package for bionic

    if(ENABLE_WORKBENCH)
      set(CPACK_DEBIAN_PACKAGE_DEPENDS
          "${CPACK_DEBIAN_PACKAGE_DEPENDS},python3-pyqt5,python3-psutil")
    endif()

    # parse list to string required for deb package
    string(REPLACE ";"
                   ","
                   CPACK_DEBIAN_PACKAGE_DEPENDS
                   ${CPACK_DEBIAN_PACKAGE_DEPENDS}) # fix up the fact that it
                                                    # was made as an array
    string(REPLACE ",,"
                   ","
                   CPACK_DEBIAN_PACKAGE_DEPENDS
                   ${CPACK_DEBIAN_PACKAGE_DEPENDS})
  endif()

  # soft requirement of jemalloc if selected
  if(USE_JEMALLOC)
    message(STATUS "Adding jemalloc to the package requirements")
    set(CPACK_RPM_PACKAGE_REQUIRES
        "${CPACK_RPM_PACKAGE_REQUIRES}, jemalloc >= 3.0")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS
        "${CPACK_DEBIAN_PACKAGE_DEPENDS}, libjemalloc1 (>= 3.0)")
  endif()

  # run cpack configuration
  include(CPack)
  # let people know what is coming out the other end - must be after cpack
  # generates value for rpm
  message(STATUS "CPACK_PACKAGE_FILE_NAME = ${CPACK_PACKAGE_FILE_NAME}")
endif()
