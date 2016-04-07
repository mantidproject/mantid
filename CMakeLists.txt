###########################################################################
# CMake version check.
# Require >= 3.3 for Windows as previous versions contained a bug that
# prevented external data from working correctly. Other systems require
# 2.8.12 to avoid recompiling system packages
###########################################################################
if (CMAKE_HOST_WIN32)
  cmake_minimum_required ( VERSION 3.3.0 )
else()
  cmake_minimum_required ( VERSION 2.8.12 )
endif()

if(POLICY CMP0053)
  #Simplify variable reference and escape sequence evaluation.
  #Claims to dramatically improve CMake configure and generate time
  #requires CMake 3.1 or later
  cmake_policy(SET CMP0053 NEW)
endif()

# Define the project name.
project ( Mantid )

# System package target is important for the windows builds as it allows us to package only the dlls and exes and exclude libs. Defaults to empty for other platforms.
set ( SYSTEM_PACKAGE_TARGET "")

# Add the path to our custom 'find' modules
set ( CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/buildconfig/CMake")

SET (CPACK_INSTALL_CMAKE_PROJECTS
     "${CMAKE_BINARY_DIR}" "Mantid" "ALL" "/")

###########################################################################
# Quick exit if we only want data targets
###########################################################################
if( DATA_TARGETS_ONLY )
  include( SetupDataTargets )
  # System test config files
  add_subdirectory( Testing/SystemTests/scripts )
  return()
endif()

set ( CPACK_PACKAGE_SUFFIX "" CACHE STRING "suffix used to determine the deployment type")
set_property(CACHE CPACK_PACKAGE_SUFFIX PROPERTY STRINGS nightly unstable "") #empty string and release are treated as the same thing

#Set package name here
set ( CPACK_PACKAGE_NAME "mantid${CPACK_PACKAGE_SUFFIX}" )

###########################################################################
# Bootstrap any dependencies
###########################################################################
include ( Bootstrap )

###########################################################################
# Set expected variables for OS X
###########################################################################
if ( ${CMAKE_SYSTEM_NAME} MATCHES "Darwin" )
  include ( DarwinSetup )
endif ()

###########################################################################
# Set working Python versions
###########################################################################
set ( Python_ADDITIONAL_VERSIONS 2.7 2.6 )

###########################################################################
# Cross-platform setup
###########################################################################
include ( CommonSetup )

###########################################################################
# Check for Qt (DarwinSetup below needs a variable from this)
###########################################################################
find_package ( Qt4 COMPONENTS QtCore QtGui QtOpenGL QtXml QtSvg Qt3Support REQUIRED )
option(WITH_QT3_SUPPORT_WARNINGS "Enable warnings about used Qt3Support functions" OFF)
if(WITH_QT3_SUPPORT_WARNINGS)
  add_definitions(-DQT3_SUPPORT_WARNINGS)
endif()

###########################################################################
# Find OpenGL
###########################################################################
find_package ( OpenGL REQUIRED )
# FindOpenGL does not (as of CMake 2.6.4) take notice of the REQUIRED
if ( NOT OPENGL_FOUND )
  message ( FATAL_ERROR "OpenGL was not found." )
endif ()

###########################################################################
# Set ParaView information since later items depend on it
###########################################################################
include ( ParaViewSetup )

# We probably don't want this to run on every build.
option(COVERALLS "Generate coveralls data" OFF)

if (COVERALLS)
    include(Coveralls)
    coveralls_turn_on_coverage()
endif()

###########################################################################
# Find qwt. Needs to be version 5
###########################################################################
find_package ( Qwt REQUIRED )
if( QWT_VERSION VERSION_LESS 5.0 OR QWT_VERSION VERSION_EQUAL 6.0 OR
    QWT_VERSION VERSION_GREATER 6.0 )
  message ( FATAL_ERROR "Qwt version 5 is required, found: ${QWT_VERSION}" )
endif()

###########################################################################
# Find PyQt & sip
###########################################################################
find_package ( PyQt4 REQUIRED )
find_package ( SIP REQUIRED )
separate_arguments ( PYQT4_SIP_FLAGS )

###########################################################################
# gsl is currently needed by Geometry, Algorithms, Curvefitting, & MantidPlot
###########################################################################
find_package ( GSL REQUIRED )

###########################################################################
# Add QtPropertyBrowser to build
###########################################################################
add_subdirectory ( QtPropertyBrowser )

###########################################################################
# Now add in all the components
###########################################################################
# Flag that this is a full build, i.e not framework only
set( FULL_PACKAGE_BUILD 1 )
# Set the path to the built docs directory here so that it can be used in
# the Kernel project for populating the properties file
set( DOCS_BUILDDIR ${CMAKE_BINARY_DIR}/docs )

# Framework Build options
set ( CXXTEST_SINGLE_LOGFILE CACHE BOOL "Switch to have the tests for each package run together")
set ( CXXTEST_ADD_PERFORMANCE OFF CACHE BOOL "Switch to add Performance tests to the list of tests run by ctest?")

add_subdirectory ( Framework )

include_directories ( Framework/Kernel/inc )
include_directories ( Framework/Geometry/inc )
include_directories ( Framework/API/inc )

set ( CORE_MANTIDLIBS Kernel Geometry API )

# Add a target for all GUI tests
add_custom_target ( GUITests )
add_dependencies ( GUITests MantidWidgetsTest)
add_dependencies ( check GUITests )
# Collect all tests together
add_custom_target ( AllTests )
add_dependencies ( AllTests FrameworkTests GUITests )

if ( UNIX )
  #Experimental feature. Unix only at this point.
  set ( UNITY_BUILD OFF CACHE BOOL "Switch for utilising unity builds. Faster builds for selected components.")
endif ( UNIX )

# VATES flag. Requires ParaView
set ( MAKE_VATES OFF CACHE BOOL "Switch for compiling the Vates project")

if ( MAKE_VATES )
  find_package( ParaView )
  add_subdirectory ( Vates )
  if(ParaView_FOUND)
    add_definitions ( -DMAKE_VATES )
  endif()
  if (NOT APPLE)
    LIST( APPEND CPACK_INSTALL_CMAKE_PROJECTS
          "${ParaView_DIR}" "ParaView Runtime Libs" "Runtime" "${INBUNDLE}/"
    )
    LIST( APPEND CPACK_INSTALL_CMAKE_PROJECTS
          "${ParaView_DIR}" "VTK Runtime Libs" "RuntimeLibraries" "${INBUNDLE}/"
    )
    LIST( APPEND CPACK_INSTALL_CMAKE_PROJECTS
          "${ParaView_DIR}" "HDF5 Core Library" "libraries" "${INBUNDLE}/"
    )
    LIST( APPEND CPACK_INSTALL_CMAKE_PROJECTS
          "${ParaView_DIR}" "HDF5 HL Library" "hllibraries" "${INBUNDLE}/"
    )
  endif()

endif ( MAKE_VATES )

if ( MSVC )
  add_definitions ( -DQWT_DLL )
endif ()
add_subdirectory ( MantidQt )

add_subdirectory ( MantidPlot )

add_subdirectory ( scripts )

add_subdirectory ( docs )

# System test data target
add_subdirectory ( Testing/SystemTests/scripts )

if (COVERALLS)
  get_property(ALL_SRCS GLOBAL PROPERTY COVERAGE_SRCS)
        set(SRCS_FILE "")
        foreach (SRC ${ALL_SRCS})
                set(SRCS_FILE "${SRCS_FILE}\n${SRC}")
        endforeach()
        #remove initial \n
        string(SUBSTRING ${SRCS_FILE} 1 -1 SRCS_FILE)
  set( SRCS_FILENAME "${CMAKE_CURRENT_BINARY_DIR}/sources.txt") 
  file(WRITE ${SRCS_FILENAME} ${SRCS_FILE})
  coveralls_setup(
    ${SRCS_FILENAME}
    OFF
    "${CMAKE_SOURCE_DIR}/buildconfig/CMake"
  )
endif ()

###########################################################################
# Installation settings
###########################################################################

# N.B. INBUNDLE variable is empty except on Mac (set in DarwinSetup.cmake)
install ( DIRECTORY installers/colormaps/ DESTINATION ${INBUNDLE}colormaps PATTERN ".svn" EXCLUDE )

# Install the files (.desktop and icon) to create a menu item, but only if installing to /opt/Mantid
if ( ${CMAKE_SYSTEM_NAME} STREQUAL "Linux" AND CMAKE_INSTALL_PREFIX STREQUAL "/opt/Mantid" )
  install ( FILES ${CMAKE_CURRENT_SOURCE_DIR}/installers/LinuxInstaller/mantidplot.desktop
            DESTINATION /usr/share/applications )
  install ( FILES ${CMAKE_CURRENT_SOURCE_DIR}/images/MantidPlot_Icon_32offset.png
            DESTINATION /usr/share/pixmaps
            RENAME mantidplot.png )
endif()

# THIS MUST BE THE LAST SUB_DIRECTORY ADDED. See Framework/PostInstall/CMakeLists
# for an explanation
add_subdirectory( Framework/PostInstall )

###########################################################################
# CPack settings
###########################################################################

# Keep this in a defaulted-to-off option
set ( ENABLE_CPACK CACHE BOOL "Switch to enable CPack package generation")
if ( ENABLE_CPACK )
    include ( CPackCommon )

    # Mac package settings
    set ( CPACK_RESOURCE_FILE_LICENSE ${CMAKE_SOURCE_DIR}/installers/WinInstaller/License.txt )

    if(CMAKE_HOST_WIN32)
      include ( WindowsNSIS )
    elseif ( ${CMAKE_SYSTEM_NAME} STREQUAL "Linux" )
      include ( CPackLinuxSetup )
      # let people know what is coming out the other end
      message ( STATUS " CPACK_PACKAGE_FILE_NAME = ${CPACK_PACKAGE_FILE_NAME}" )

      # rhel requirements
      set ( CPACK_RPM_PACKAGE_REQUIRES "qt4 >= 4.2,nexus >= 4.3.1,nexus-python >= 4.3.1,gsl,glibc,qwtplot3d-qt4,muParser,numpy" )
      # OCE
      set( CPACK_RPM_PACKAGE_REQUIRES "${CPACK_RPM_PACKAGE_REQUIRES},OCE-draw,OCE-foundation,OCE-modeling,OCE-ocaf,OCE-visualization")
      set ( CPACK_RPM_PACKAGE_REQUIRES "${CPACK_RPM_PACKAGE_REQUIRES},poco-crypto,poco-data,poco-mysql,poco-sqlite,poco-odbc,poco-util,poco-xml,poco-zip,poco-net,poco-netssl,poco-foundation,PyQt4,sip" )
      set ( CPACK_RPM_PACKAGE_REQUIRES "${CPACK_RPM_PACKAGE_REQUIRES},python-ipython >= 1.1.0,python-ipython-notebook" )
      # scipy & matplotlib
      set ( CPACK_RPM_PACKAGE_REQUIRES "${CPACK_RPM_PACKAGE_REQUIRES},scipy,python-matplotlib" )
      set ( CPACK_RPM_PACKAGE_REQUIRES "${CPACK_RPM_PACKAGE_REQUIRES},mxml,hdf,hdf5,jsoncpp >= 0.7.0" )

      if( "${UNIX_CODENAME}" MATCHES "Santiago" )
        # On RHEL6 we have to use an updated qscintilla to fix an auto complete bug
        set ( CPACK_RPM_PACKAGE_REQUIRES "${CPACK_RPM_PACKAGE_REQUIRES} qscintilla >= 2.4.6, boost157" )
        # On RHEL6 we are using SCL packages for Qt
        set ( CPACK_RPM_PACKAGE_REQUIRES "${CPACK_RPM_PACKAGE_REQUIRES},scl-utils,mantidlibs34,mantidlibs34-runtime,mantidlibs34-qt,mantidlibs34-qt-x11,mantidlibs34-qt-webkit,mantidlibs34-qwt5-qt4" )
      else()
        set ( CPACK_RPM_PACKAGE_REQUIRES "${CPACK_RPM_PACKAGE_REQUIRES} qscintilla,qwt5-qt4,python-matplotlib-qt4,boost >= 1.53.0" )
      endif()

      # Add software collections for RHEL
      if ( "${UNIX_CODENAME}" MATCHES "Santiago" )
        set ( CPACK_RPM_PACKAGE_REQUIRES "${CPACK_RPM_PACKAGE_REQUIRES} scl-utils" )
      endif()

      if( "${UNIX_DIST}" MATCHES "Ubuntu" )
        # common packages
        set ( DEPENDS_LIST "libboost-date-time${Boost_MAJOR_VERSION}.${Boost_MINOR_VERSION}.${Boost_SUBMINOR_VERSION},"
                           "libboost-regex${Boost_MAJOR_VERSION}.${Boost_MINOR_VERSION}.${Boost_SUBMINOR_VERSION},"
                           "libboost-python${Boost_MAJOR_VERSION}.${Boost_MINOR_VERSION}.${Boost_SUBMINOR_VERSION},"
                           "libnexus0 (>= 4.3),"
                           "libgsl0ldbl,"
                           "libqtcore4 (>= 4.2),"
                           "libqtgui4 (>= 4.2),"
                           "libqt4-opengl (>= 4.2),"
                           "libqt4-xml (>= 4.2),"
                           "libqt4-svg (>= 4.2),"
                           "libqt4-qt3support (>= 4.2),"
                           "qt4-dev-tools,"
                           "libqwt5-qt4,"
                           "libqwtplot3d-qt4-0,"
                           "python-numpy,"
                           "python-sip,"
                           "python-qt4,"
                           "python-nxs (>= 4.3),"
                           "libjsoncpp0 (>=0.7.0),"
                           "libqscintilla2-11,"
                           "liboce-foundation8,liboce-modeling8,"
                           "libmuparser2,"
                           "ipython-qtconsole (>= 1.1),"
                           "ipython-notebook,"
                           "python-matplotlib,"
                           "python-scipy,"
                           "libpocofoundation11,libpocoutil11,libpoconet11,libpoconetssl11,libpococrypto11,libpocoxml11" )
        set ( PERFTOOLS_DEB_PACKAGE "libgoogle-perftools4 (>= 1.7)" )
        if( "${UNIX_CODENAME}" STREQUAL "wily"  OR "${UNIX_CODENAME}" STREQUAL "xenial")
            list ( APPEND DEPENDS_LIST ", libnexus0v5 (>= 4.3),libjsoncpp0v5 (>= 0.7.0),libqscintilla2-12v5, libmuparser2v5,libqwtplot3d-qt4-0v5")
            list (REMOVE_ITEM DEPENDS_LIST "libjsoncpp0 (>=0.7.0)," "libqscintilla2-11," "libmuparser2," "libnexus0 (>= 4.3)," "libqwtplot3d-qt4-0,")
        endif()
        # parse list to string required for deb package
        string ( REPLACE ";" "" CPACK_DEBIAN_PACKAGE_DEPENDS ${DEPENDS_LIST} )
      endif()
      # soft requirement of tcmalloc if selected
      IF ( USE_TCMALLOC )
        message ( STATUS " Adding gperftools to the package requirements" )
        set ( CPACK_RPM_PACKAGE_REQUIRES "${CPACK_RPM_PACKAGE_REQUIRES},gperftools-libs >= 2.0" )
        set ( CPACK_DEBIAN_PACKAGE_DEPENDS "${CPACK_DEBIAN_PACKAGE_DEPENDS},${PERFTOOLS_DEB_PACKAGE}" )
      ENDIF ( )
    ENDIF ()
    # run cpack configuration
    include ( CPack )
endif ()
