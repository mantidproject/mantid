# CMake version check. Require CMake 3.5, required by ParaView 5.1.0
cmake_minimum_required(VERSION 3.5)

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

option(ENABLE_MANTIDPLOT "Enable Qt4-based gui & components" ON)
option(ENABLE_WORKBENCH "Enable Qt5-based gui & components" ON)

set(CPACK_INSTALL_CMAKE_PROJECTS "${CMAKE_BINARY_DIR}" "Mantid" "ALL" "/")

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

# Set package name here
set(CPACK_PACKAGE_NAME "mantid${CPACK_PACKAGE_SUFFIX}")

# Bootstrap any dependencies
include(Bootstrap)

# Configure Eigen early
include(Eigen)

include(Span)

# Set ParaView information since later items depend on it

# VATES flag. Requires ParaView
option(MAKE_VATES "Switch for compiling the Vates project")
if(ENABLE_MANTIDPLOT AND MAKE_VATES)
  find_package(ParaView)
  if(ParaView_FOUND)
    add_definitions(-DMAKE_VATES)
    # Poco::File throws an exception when a backslash is used.
    string(REPLACE "\\"
                   "/"
                   ParaView_DIR
                   ${ParaView_DIR})
  endif()
endif()
if(ENABLE_MANTIDPLOT)
  include(ParaViewSetup)
endif()

# Set expected variables for OS X
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  include(DarwinSetup)
endif()

# Cross-platform setup
include(CommonSetup)

# Find OpenGL
find_package(OpenGL REQUIRED)
# FindOpenGL does not (as of CMake 2.6.4) take notice of the REQUIRED
if(NOT OPENGL_FOUND)
  message(FATAL_ERROR "OpenGL was not found.")
endif()

find_package(TBB REQUIRED)

# We probably don't want this to run on every build.
option(COVERALLS "Generate coveralls data" OFF)

if(COVERALLS)
  include(Coveralls)
  coveralls_turn_on_coverage()
endif()

# Qt/Qwt/PyQt/sip
if(ENABLE_MANTIDPLOT)
  set(QT_USE_IMPORTED_TARGETS ON)
  find_package(Qt4
               COMPONENTS QtCore
                          QtGui
                          QtOpenGL
                          QtXml
                          QtSvg
               REQUIRED)

  find_package(Qwt5 REQUIRED)
  if(QWT5_VERSION VERSION_LESS 5.0
     OR QWT5_VERSION VERSION_EQUAL 6.0
     OR QWT5_VERSION VERSION_GREATER 6.0)
    message(FATAL_ERROR "Qwt version 5 is required, found: ${QWT5_VERSION}")
  endif()

  find_package(PyQt4 REQUIRED)
  find_package(SIP REQUIRED)
  separate_arguments(PYQT4_SIP_FLAGS)
endif()

if(ENABLE_WORKBENCH)
  find_package(Qt5
               COMPONENTS Core
                          Gui
                          Widgets
                          OpenGL
               REQUIRED)
  if(Qt5_FOUND)
    message(STATUS "Found Qt ${Qt5_VERSION}: ${Qt5_DIR}")
  endif()
  find_package(PyQt5 REQUIRED)
  find_package(SIP REQUIRED)
  separate_arguments(PYQT5_SIP_FLAGS)
endif()

# gsl is currently needed by Geometry, Algorithms, Curvefitting, & MantidPlot
find_package(GSL REQUIRED)

# Now add in all the components

# Flag that this is a full build, i.e not framework only
set(FULL_PACKAGE_BUILD 1)
# Set the path to the built docs directory here so that it can be used in the
# Kernel project for populating the properties file
set(DOCS_BUILDDIR ${CMAKE_BINARY_DIR}/docs)

# Framework Build options
option(CXXTEST_ADD_PERFORMANCE
       "Switch to add Performance tests to the list of tests run by ctest?")

add_subdirectory(Framework)

include_directories(Framework/Kernel/inc)
include_directories(Framework/HistogramData/inc)
include_directories(Framework/Indexing/inc)
include_directories(Framework/Parallel/inc)
include_directories(Framework/Beamline/inc)
include_directories(Framework/Geometry/inc)
include_directories(Framework/API/inc)
include_directories(Framework/Types/inc)
include_directories(Framework/NexusGeometry/inc)

set(CORE_MANTIDLIBS Kernel HistogramData Indexing Beamline Geometry API Types)

if(ENABLE_MANTIDPLOT AND MAKE_VATES)

  if(NOT APPLE)
    list(APPEND CPACK_INSTALL_CMAKE_PROJECTS
                "${ParaView_DIR}"
                "ParaView Runtime Libs"
                "Runtime"
                "${INBUNDLE}/")
    list(APPEND CPACK_INSTALL_CMAKE_PROJECTS
                "${ParaView_DIR}"
                "VTK Runtime Libs"
                "RuntimeLibraries"
                "${INBUNDLE}/")
    list(APPEND CPACK_INSTALL_CMAKE_PROJECTS
                "${ParaView_DIR}"
                "HDF5 Core Library"
                "libraries"
                "${INBUNDLE}/")
    list(APPEND CPACK_INSTALL_CMAKE_PROJECTS
                "${ParaView_DIR}"
                "HDF5 HL Library"
                "hllibraries"
                "${INBUNDLE}/")
  endif()

endif(ENABLE_MANTIDPLOT AND MAKE_VATES)

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

if(ENABLE_MANTIDPLOT OR ENABLE_WORKBENCH)
  add_custom_target(GUITests)
  add_dependencies(check GUITests)
  # Collect all tests together
  add_dependencies(AllTests GUITests)
  add_subdirectory(qt)
endif()

if(ENABLE_MANTIDPLOT)
  add_subdirectory(MantidPlot)
endif()

add_subdirectory(scripts)

# Docs requirements
option(ENABLE_DOCS "Enable Building user and developer documentation" ON)
if (ENABLE_DOCS)
  find_package(Sphinx 1.2)
  if(SPHINX_FOUND)
    # run python to see if the theme is installed
    execute_process(COMMAND ${PYTHON_EXECUTABLE} -c
                            "import sphinx_bootstrap_theme"
                    OUTPUT_VARIABLE SPHINX_BOOTSTRAP_THEME_OUT
                    ERROR_VARIABLE SPHINX_BOOTSTRAP_THEME_ERR
                    OUTPUT_STRIP_TRAILING_WHITESPACE
                    ERROR_STRIP_TRAILING_WHITESPACE)
    if(SPHINX_BOOTSTRAP_THEME_ERR)
      message(ERROR " Did not find sphinx_bootstrap_theme")
      message(STATUS "${PYTHON_EXECUTABLE} -c \"import sphinx_bootstrap_theme\"")
      message(STATUS "${SPHINX_BOOTSTRAP_THEME_ERR}")
      message(
        FATAL_ERROR
          " Install instructions at https://pypi.python.org/pypi/sphinx-bootstrap-theme/"
        )
    endif()

    add_subdirectory(dev-docs)
    if(ENABLE_MANTIDPLOT)
      add_subdirectory(docs)
    endif()
  endif()
endif()

# System test data target
add_subdirectory(Testing/SystemTests/scripts)

if(COVERALLS)
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
  coveralls_setup(${SRCS_FILENAME} OFF "${CMAKE_SOURCE_DIR}/buildconfig/CMake")
endif()

# Installation settings

# N.B. INBUNDLE variable is empty except on Mac (set in DarwinSetup.cmake)
install(DIRECTORY installers/colormaps/ DESTINATION ${INBUNDLE}colormaps)

# Install the files (.desktop and icon) to create a menu items
if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  function(uppercase_first_letter output_var input)
  string(LENGTH ${input} _inputlen)
  string(TOUPPER ${input} _allupper)
  string(SUBSTRING ${_allupper} 0 1 _first)
  string(SUBSTRING ${input} 1 ${_inputlen} _rest)
  set(${output_var} ${_first}${_rest} PARENT_SCOPE)
  endfunction()

  function(install_desktop_files filename app_name exec_cmd icon_file)
    set(DESKTOP_NAME ${app_name})
    set(DESKTOP_EXEC_CMD ${exec_cmd})
    get_filename_component(DESKTOP_ICON ${icon_file} NAME_WE)
    set(_output_file ${CMAKE_CURRENT_BINARY_DIR}/${filename}.install)
    configure_file(
      ${CMAKE_CURRENT_SOURCE_DIR}/installers/LinuxInstaller/launcher-template.desktop.in
      ${_output_file} @ONLY)
    install(FILES ${_output_file}
            DESTINATION /usr/share/applications
            RENAME ${filename})
    install(FILES ${icon_file}
            DESTINATION /usr/share/pixmaps
            RENAME ${DESKTOP_ICON}.png)
  endfunction()

  set( IMAGES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/images )
  set(_app_name_suffix)
  if(CPACK_PACKAGE_SUFFIX)
    uppercase_first_letter(_app_name_suffix ${CPACK_PACKAGE_SUFFIX})
  endif()
  if(ENABLE_MANTIDPLOT)
    set(_icon_filename ${IMAGES_DIR}/mantidplot)
    if(CPACK_PACKAGE_SUFFIX)
      set(_icon_filename ${_icon_filename}${CPACK_PACKAGE_SUFFIX})
    endif()
    install_desktop_files(mantidplot${CPACK_PACKAGE_SUFFIX}.desktop
                          MantidPlot${_app_name_suffix}
                          ${CMAKE_INSTALL_PREFIX}/bin/MantidPlot
                          ${_icon_filename}.png)
  endif()

  if(ENABLE_WORKBENCH)
    set(_icon_filename ${IMAGES_DIR}/mantid_workbench)
    if(CPACK_PACKAGE_SUFFIX)
      set(_icon_filename ${_icon_filename}${CPACK_PACKAGE_SUFFIX})
    endif()
    install_desktop_files(mantidworkbench${CPACK_PACKAGE_SUFFIX}.desktop
                          MantidWorkbench${_app_name_suffix}
                          ${CMAKE_INSTALL_PREFIX}/bin/mantidworkbench
                          ${_icon_filename}.png)
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
      "nexus >= 4.3.1,gsl,glibc,muParser,numpy,h5py >= 2.3.1,PyCifRW >= 4.2.1,tbb,librdkafka,"
      "${CPACK_RPM_PACKAGE_REQUIRES},OCE-draw,OCE-foundation,OCE-modeling,OCE-ocaf,OCE-visualization,"
      "poco-crypto,poco-data,poco-mysql,poco-sqlite,poco-odbc,poco-util,poco-xml,poco-zip,poco-net,poco-netssl,poco-foundation,"
      "sip >= 4.18,"
      "python-six,python-ipython >= 1.1.0,python-ipython-notebook,PyYAML,"
      "python-requests,"
      "scipy,python2-scikit-image,"
      "hdf,hdf5,jsoncpp >= 0.7.0")
    if(ENABLE_MANTIDPLOT)
      set(CPACK_RPM_PACKAGE_REQUIRES
          "${CPACK_RPM_PACKAGE_REQUIRES},qt4 >= 4.2,PyQt4,qwtplot3d-qt4")
    endif()
    if(ENABLE_WORKBENCH)
      set(CPACK_RPM_PACKAGE_REQUIRES
          "${CPACK_RPM_PACKAGE_REQUIRES},qt5-qtbase,python-qt5")
    endif()

    if("${UNIX_CODENAME}" MATCHES "Santiago") # RHEL6
      if(ENABLE_WORKBENCH)
        message(FATAL_ERROR "mantidworkbench is not supported on RHEL6")
      endif()
      # On RHEL6 we have to use an updated qscintilla to fix an auto complete
      # bug
      set(
        CPACK_RPM_PACKAGE_REQUIRES
        "${CPACK_RPM_PACKAGE_REQUIRES},qscintilla >= 2.4.6,boost157,python-matplotlib,"
        # On RHEL6 we are using SCL packages for Qt
        "scl-utils,mantidlibs34,mantidlibs34-runtime,mantidlibs34-qt,mantidlibs34-qt-x11,mantidlibs34-qt-webkit,mantidlibs34-qwt5-qt4"
        )
    else() # assumes RHEL7 Require matplotlib >= 1.5 to fix bug in latex
           # rendering - oddly matplotlib-qt4 contains qt5 backend as well
      set(
        CPACK_RPM_PACKAGE_REQUIRES
        "${CPACK_RPM_PACKAGE_REQUIRES},boost >= 1.53.0,python2-matplotlib-qt4 >= 1.5.2"
        )
      if(ENABLE_MANTIDPLOT)
        set(CPACK_RPM_PACKAGE_REQUIRES
            "${CPACK_RPM_PACKAGE_REQUIRES},qscintilla,qwt5-qt4")
      endif()
      if(ENABLE_WORKBENCH)
        set(CPACK_RPM_PACKAGE_REQUIRES
            "${CPACK_RPM_PACKAGE_REQUIRES},qscintilla-qt5,python-psutil")
      endif()
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
    if(ENABLE_MANTIDPLOT)
      set(CPACK_DEBIAN_PACKAGE_DEPENDS
          "${CPACK_DEBIAN_PACKAGE_DEPENDS},libqtcore4 (>= 4.2),"
          "libqtgui4 (>= 4.2),"
          "libqt4-opengl (>= 4.2),"
          "libqt4-xml (>= 4.2),"
          "libqt4-svg (>= 4.2),"
          "libqt4-qt3support (>= 4.2),"
          "qt4-dev-tools,"
          "libqwt5-qt4,"
          "libqwtplot3d-qt4-0v5,"
          "libqtwebkit4")
    endif()

    if("${UNIX_CODENAME}" STREQUAL "xenial")
      set(CPACK_DEBIAN_PACKAGE_DEPENDS
          "${CPACK_DEBIAN_PACKAGE_DEPENDS},libgsl2,"
          "liboce-foundation10,liboce-modeling10,"
          "libqscintilla2-12v5,"
          "ipython-notebook,"
          "libhdf5-cpp-11")
      if(ENABLE_WORKBENCH)
        set(CPACK_DEBIAN_PACKAGE_DEPENDS
            "${CPACK_DEBIAN_PACKAGE_DEPENDS},libqt5scintilla2-12v5")
      endif()
    else() # bionic and newer
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
    endif()

    if(PYTHON_VERSION_MAJOR EQUAL 3)
      set(CPACK_DEBIAN_PACKAGE_DEPENDS
          "${CPACK_DEBIAN_PACKAGE_DEPENDS},python3-h5py,"
          "python3-numpy,"
          "python3-requests,"
          "python3-sip,"
          "python3-six,"
          "python3-matplotlib,"
          "python3-scipy,"
          "python3-skimage,"
          "python3-pycifrw (>= 4.2.1),"
          "python3-yaml,"
          "ipython3-qtconsole") # transitional package for bionic

      if(ENABLE_MANTIDPLOT)
        set(CPACK_DEBIAN_PACKAGE_DEPENDS
            "${CPACK_DEBIAN_PACKAGE_DEPENDS},python3-pyqt4")
      endif()
      if(ENABLE_WORKBENCH)
        set(CPACK_DEBIAN_PACKAGE_DEPENDS
            "${CPACK_DEBIAN_PACKAGE_DEPENDS},python3-pyqt5,python3-psutil")
      endif()
    else() # python 2
      set(CPACK_DEBIAN_PACKAGE_DEPENDS
          "${CPACK_DEBIAN_PACKAGE_DEPENDS},python-h5py,"
          "python-numpy,"
          "python-requests,"
          "python-sip,"
          "python-six,"
          "python-matplotlib,"
          "python-scipy,"
          "python-skimage,"
          "python-pycifrw (>= 4.2.1),"
          "python-yaml,"
          "ipython-qtconsole")

      if(ENABLE_MANTIDPLOT)
        set(CPACK_DEBIAN_PACKAGE_DEPENDS
            "${CPACK_DEBIAN_PACKAGE_DEPENDS},python-qt4")
      endif()
      if(ENABLE_WORKBENCH)
        set(CPACK_DEBIAN_PACKAGE_DEPENDS
            "${CPACK_DEBIAN_PACKAGE_DEPENDS},python-pyqt5,python-psutil")
      endif()
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

  # soft requirement of tcmalloc if selected
  if(USE_TCMALLOC)
    message(STATUS "Adding gperftools to the package requirements")
    set(CPACK_RPM_PACKAGE_REQUIRES
        "${CPACK_RPM_PACKAGE_REQUIRES},gperftools-libs >= 2.0")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS
        "${CPACK_DEBIAN_PACKAGE_DEPENDS},libgoogle-perftools4 (>= 1.7)")
  endif()

  # run cpack configuration
  include(CPack)
  # let people know what is coming out the other end - must be after cpack
  # generates value for rpm
  message(STATUS "CPACK_PACKAGE_FILE_NAME = ${CPACK_PACKAGE_FILE_NAME}")
endif()
