###########################################################################
# CMake version check.
# Require CMake 3.5, required by ParaView 5.1.0
###########################################################################
cmake_minimum_required ( VERSION 3.5 )

# Define the project name.
project ( Mantid )

# System package target is important for the windows builds as it allows us to package only the dlls and exes and exclude libs. Defaults to empty for other platforms.
set ( SYSTEM_PACKAGE_TARGET "")

# Add the path to our custom 'find' modules
set ( CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/buildconfig/CMake")

set ( ENABLE_MANTIDPLOT ON CACHE BOOL "Switch for compiling the gui components")

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
# Include Eigen as an external project
###########################################################################
include ( Eigen )

###########################################################################
# Bootstrap any dependencies
###########################################################################
include ( Bootstrap )

###########################################################################
# Set ParaView information since later items depend on it
###########################################################################
# VATES flag. Requires ParaView
set ( MAKE_VATES OFF CACHE BOOL "Switch for compiling the Vates project")
if ( ENABLE_MANTIDPLOT AND MAKE_VATES )
  find_package( ParaView )
  if( ParaView_FOUND )
    add_definitions ( -DMAKE_VATES )
    # Poco::File throws an exception when a backslash is used.
    string ( REPLACE "\\" "/" ParaView_DIR ${ParaView_DIR} )
  endif ()
endif ()
if ( ENABLE_MANTIDPLOT )
  include ( ParaViewSetup )
endif ()

###########################################################################
# Set expected variables for OS X
###########################################################################
if ( ${CMAKE_SYSTEM_NAME} MATCHES "Darwin" )
  include ( DarwinSetup )
endif ()

###########################################################################
# Cross-platform setup
###########################################################################
include ( CommonSetup )

###########################################################################
# Check for Qt (DarwinSetup below needs a variable from this)
###########################################################################
if (ENABLE_MANTIDPLOT)
  find_package ( Qt4 COMPONENTS QtCore QtGui QtOpenGL QtXml QtSvg REQUIRED )
endif ()

###########################################################################
# Find OpenGL
###########################################################################
find_package ( OpenGL REQUIRED )
# FindOpenGL does not (as of CMake 2.6.4) take notice of the REQUIRED
if ( NOT OPENGL_FOUND )
  message ( FATAL_ERROR "OpenGL was not found." )
endif ()

find_package(TBB REQUIRED)

# We probably don't want this to run on every build.
option(COVERALLS "Generate coveralls data" OFF)

if (COVERALLS)
    include(Coveralls)
    coveralls_turn_on_coverage()
endif()

###########################################################################
# Find qwt. Needs to be version 5
###########################################################################
if (ENABLE_MANTIDPLOT)
  find_package ( Qwt REQUIRED )
  if( QWT_VERSION VERSION_LESS 5.0 OR QWT_VERSION VERSION_EQUAL 6.0 OR
      QWT_VERSION VERSION_GREATER 6.0 )
    message ( FATAL_ERROR "Qwt version 5 is required, found: ${QWT_VERSION}" )
  endif()
endif()

###########################################################################
# Find PyQt & sip
###########################################################################
if (ENABLE_MANTIDPLOT)
  find_package ( PyQt4 REQUIRED )
  find_package ( SIP REQUIRED )
  separate_arguments ( PYQT4_SIP_FLAGS )
endif()

###########################################################################
# gsl is currently needed by Geometry, Algorithms, Curvefitting, & MantidPlot
###########################################################################
find_package ( GSL REQUIRED )

###########################################################################
# Add QtPropertyBrowser to build
###########################################################################
if (ENABLE_MANTIDPLOT)
  add_subdirectory ( QtPropertyBrowser )
endif(ENABLE_MANTIDPLOT)

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
include_directories ( Framework/HistogramData/inc )
include_directories ( Framework/Indexing/inc )
include_directories ( Framework/Geometry/inc )
include_directories ( Framework/API/inc )
include_directories ( Framework/Types/inc )

set ( CORE_MANTIDLIBS Kernel HistogramData Indexing Geometry API Types )

if (ENABLE_MANTIDPLOT)
  # Add a target for all GUI tests
  add_custom_target ( GUITests )
  add_dependencies ( GUITests MantidWidgetsTest)
  add_dependencies ( check GUITests )
  # Collect all tests together
  add_custom_target ( AllTests )
  add_dependencies ( AllTests FrameworkTests GUITests )
endif()

if ( UNIX )
  #Experimental feature. Unix only at this point.
  set ( UNITY_BUILD OFF CACHE BOOL "Switch for utilising unity builds. Faster builds for selected components.")
endif ( UNIX )

if (ENABLE_MANTIDPLOT AND MAKE_VATES )
  add_subdirectory ( Vates )
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

endif ( ENABLE_MANTIDPLOT AND MAKE_VATES )

if ( MSVC )
  add_definitions ( -DQWT_DLL )
endif ()

if (ENABLE_MANTIDPLOT)
  add_subdirectory ( MantidQt )

  add_subdirectory ( MantidPlot )
endif()

add_subdirectory ( scripts )

if (ENABLE_MANTIDPLOT)
  add_subdirectory ( docs )
endif()

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
install ( DIRECTORY installers/colormaps/ DESTINATION ${INBUNDLE}colormaps)

# Install the files (.desktop and icon) to create a menu item, but only if installing to /opt/Mantid
if ( ENABLE_MANTIDPLOT AND ${CMAKE_SYSTEM_NAME} STREQUAL "Linux" AND CMAKE_INSTALL_PREFIX STREQUAL "/opt/Mantid" )
  install ( FILES ${CMAKE_CURRENT_SOURCE_DIR}/installers/LinuxInstaller/mantidplot.desktop
            DESTINATION /usr/share/applications )
  install ( FILES ${CMAKE_CURRENT_SOURCE_DIR}/images/MantidPlot_Icon_32offset.png
            DESTINATION /usr/share/pixmaps
            RENAME mantidplot.png )
endif()

# THIS MUST BE THE LAST SUB_DIRECTORY ADDED. See Framework/PostInstall/CMakeLists
# for an explanation
add_subdirectory( Framework/PostInstall )


# Conda
set ( ENABLE_CONDA CACHE BOOL "Switch to enable conda package generation")
if ( ENABLE_CONDA )
  include ( conda )
endif ( ENABLE_CONDA )

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
      set ( CPACK_RPM_PACKAGE_REQUIRES "qt4 >= 4.2,nexus >= 4.3.1,nexus-python >= 4.3.1,gsl,glibc,qwtplot3d-qt4,muParser,numpy,h5py >= 2.3.1,PyCifRW >= 4.2.1,tbb,librdkafka" )
      # OCE
      set( CPACK_RPM_PACKAGE_REQUIRES "${CPACK_RPM_PACKAGE_REQUIRES},OCE-draw,OCE-foundation,OCE-modeling,OCE-ocaf,OCE-visualization")
      set ( CPACK_RPM_PACKAGE_REQUIRES "${CPACK_RPM_PACKAGE_REQUIRES},poco-crypto,poco-data,poco-mysql,poco-sqlite,poco-odbc,poco-util,poco-xml,poco-zip,poco-net,poco-netssl,poco-foundation,PyQt4,sip" )
      set ( CPACK_RPM_PACKAGE_REQUIRES "${CPACK_RPM_PACKAGE_REQUIRES},python-six,python-ipython >= 1.1.0,python-ipython-notebook,PyYAML" )
      # scipy
      set ( CPACK_RPM_PACKAGE_REQUIRES "${CPACK_RPM_PACKAGE_REQUIRES},scipy" )
      set ( CPACK_RPM_PACKAGE_REQUIRES "${CPACK_RPM_PACKAGE_REQUIRES},mxml,hdf,hdf5,jsoncpp >= 0.7.0" )

      if( "${UNIX_CODENAME}" MATCHES "Santiago" )
        # On RHEL6 we have to use an updated qscintilla to fix an auto complete bug
        set ( CPACK_RPM_PACKAGE_REQUIRES "${CPACK_RPM_PACKAGE_REQUIRES} qscintilla >= 2.4.6, boost157,python-matplotlib" )
        # On RHEL6 we are using SCL packages for Qt
        set ( CPACK_RPM_PACKAGE_REQUIRES "${CPACK_RPM_PACKAGE_REQUIRES},scl-utils,mantidlibs34,mantidlibs34-runtime,mantidlibs34-qt,mantidlibs34-qt-x11,mantidlibs34-qt-webkit,mantidlibs34-qwt5-qt4" )
      else()
        # Require matplotlib >= 1.5 to fix bug in latex rendering
        set ( CPACK_RPM_PACKAGE_REQUIRES "${CPACK_RPM_PACKAGE_REQUIRES} qscintilla,qwt5-qt4,python2-matplotlib-qt4 >= 1.5.2,boost >= 1.53.0" )
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
                           "python-h5py,"
                           "python-numpy,"
                           "python-sip,"
                           "python-qt4,"
                           "python-nxs (>= 4.3),"
                           "python-six,"
                           "libjsoncpp0 (>=0.7.0),"
                           "libqscintilla2-11,"
                           "liboce-foundation8,liboce-modeling8,"
                           "libmuparser2,"
                           "ipython-qtconsole (>= 1.1),"
                           "ipython-notebook,"
                           "python-matplotlib,"
                           "python-scipy,"
                           "libtbb2,"
                           "librdkafka1,"
                           "librdkafka++1,"
                           "libpocofoundation${POCO_SOLIB_VERSION},libpocoutil${POCO_SOLIB_VERSION},libpoconet${POCO_SOLIB_VERSION},libpoconetssl${POCO_SOLIB_VERSION},libpococrypto${POCO_SOLIB_VERSION},libpocoxml${POCO_SOLIB_VERSION},"
                           "python-pycifrw (>= 4.2.1),"
                           "python-yaml")
        set ( PERFTOOLS_DEB_PACKAGE "libgoogle-perftools4 (>= 1.7)" )
        if( "${UNIX_CODENAME}" STREQUAL "xenial")
            list ( APPEND DEPENDS_LIST ", libhdf5-cpp-11,libnexus0v5 (>= 4.3),libjsoncpp1,libqscintilla2-12v5, libmuparser2v5,libqwtplot3d-qt4-0v5,libgsl2,liboce-foundation10,liboce-modeling10")
            list (REMOVE_ITEM DEPENDS_LIST "libjsoncpp0 (>=0.7.0)," "libqscintilla2-11," "libmuparser2," "libnexus0 (>= 4.3)," "libqwtplot3d-qt4-0," "libgsl0ldbl," "liboce-foundation8,liboce-modeling8,")
        endif()
        # Change DEPENDS_LIST for python 3
        if ( PYTHON_VERSION_MAJOR EQUAL 3 )
          list (REMOVE_ITEM DEPENDS_LIST "python-nxs (>= 4.3)," "python-pycifrw (>= 4.2.1),")
          string ( REPLACE "python-" "python3-" DEPENDS_LIST ${DEPENDS_LIST} )
          string ( REPLACE "python3-qt4" "python3-pyqt4" DEPENDS_LIST ${DEPENDS_LIST} )
        endif ()
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
