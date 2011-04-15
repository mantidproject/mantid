# This is mainly here so you don't get a complaint when running cmake
cmake_minimum_required ( VERSION 2.6 )

# Add the path to our custom 'find' modules
set ( CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/Build/CMake")

# Define the project name.
project ( Mantid )

###########################################################################
# Set paths to Third_Party for Windows and Mac builds
###########################################################################
if ( NOT THIRD_PARTY )
  set ( THIRD_PARTY "${PROJECT_SOURCE_DIR}/../Third_Party" )
  if ( WIN32 )
    include ( WindowsSetup )
  elseif ( APPLE )
    include ( DarwinSetup )
  endif ()
endif ()

###########################################################################
# Call our setup script
###########################################################################
include ( CommonSetup )

###########################################################################
# Add linux-specific things (N.B. 'UNIX' variable is true on OSX)
###########################################################################
if ( ${CMAKE_SYSTEM_NAME} STREQUAL "Linux" )
  include ( LinuxSetup )
endif ()

###########################################################################
# Add compiler options if using gcc
###########################################################################
if ( CMAKE_COMPILER_IS_GNUCXX )
  include ( GNUSetup )
endif ()

###########################################################################
# Check for Qt
###########################################################################
find_package ( Qt4 COMPONENTS QtCore QtGui QtOpenGL QtXml QtSvg Qt3Support REQUIRED )
include ( ${QT_USE_FILE} )

###########################################################################
# Now qwt
###########################################################################
find_package ( Qwt REQUIRED )

###########################################################################
# Add QtPropertyBrowser to build
###########################################################################
add_subdirectory ( QtPropertyBrowser )

###########################################################################
# Now add in all the components
###########################################################################
add_subdirectory ( Framework )

include_directories ( Framework/Kernel/inc )
include_directories ( Framework/Geometry/inc )
include_directories ( Framework/API/inc )
set ( CORE_MANTIDLIBS Kernel Geometry API )

add_subdirectory ( MantidQt )

include_directories ( QtPropertyBrowser/src )
include_directories ( MantidQt/API/inc )
include_directories ( MantidQt/MantidWidgets/inc )
include_directories ( ${CMAKE_CURRENT_BINARY_DIR}/MantidQt/API )
include_directories ( ${CMAKE_CURRENT_BINARY_DIR}/MantidQt/MantidWidgets )

add_subdirectory ( MantidPlot )

if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  #Experimental feature. Linux only at this point.
  set ( UNITY_BUILD OFF CACHE BOOL "Switch for utilising unity builds. Faster builds for selected components.")
endif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")

set ( MAKE_VATES OFF CACHE BOOL "Switch for compiling the Vates project")
set ( REDUCER_TEST OFF CACHE BOOL "Switch for running flaky Reducer test")
set ( MAKE_MDEVENTS OFF CACHE BOOL "Switch for compiling the MDEvents project")
set ( CXXTEST_SINGLE_LOGFILE CACHE BOOL "Switch to have the tests for each package run together")
set ( CXXTEST_ADD_PERFORMANCE OFF CACHE BOOL "Switch to add Performance tests to the list of tests run by ctest?") 

if ( MAKE_VATES )
  add_subdirectory ( Vates )
endif ( MAKE_VATES )

# python unit tests
if (PYUNITTEST_FOUND)
  add_subdirectory ( scripts )
endif ()

###########################################################################
# Installation settings
###########################################################################

# Create instrument directory and make it writable so vtp files can go there
install ( DIRECTORY DESTINATION instrument 
          DIRECTORY_PERMISSIONS OWNER_READ OWNER_EXECUTE OWNER_WRITE
                                GROUP_READ GROUP_EXECUTE GROUP_WRITE
                                WORLD_READ WORLD_EXECUTE WORLD_WRITE
)
file ( GLOB INSTRUMENT_FILES instrument/*.xml )
install ( FILES ${INSTRUMENT_FILES} DESTINATION instrument )
install ( DIRECTORY scripts/ DESTINATION scripts PATTERN ".svn" EXCLUDE 
                                                 PATTERN "*.pyc" EXCLUDE )
install ( DIRECTORY Installers/colormaps/ DESTINATION colormaps PATTERN ".svn" EXCLUDE )

###########################################################################
# CPack settings
###########################################################################

# Keep this in a defaulted-to-off option
set ( ENABLE_CPACK CACHE BOOL "Switch to enable CPack package generation")
if ( ENABLE_CPACK )
  # generic package level information
  set ( CPACK_PACKAGE_DESCRIPTION_SUMMARY "Neutron Scattering Data Analysis" )
  set ( CPACK_PACKAGE_VENDOR "ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory" )
  set ( CPACK_PACKAGE_URL http://www.mantidproject.org/ )
  set ( CPACK_PACKAGE_CONTACT mantid-help@mantidproject.org )
  set ( CPACK_PACKAGE_ICON Images/MantidPlot_Icon_32offset.png )
  set ( CPACK_PACKAGE_NAME mantid )
  set ( CPACK_PACKAGE_VERSION 1.1.${Framework_WC_REVISION} )
  set ( CPACK_PACKAGE_VERSION_MAJOR 1 )
  set ( CPACK_PACKAGE_VERSION_MINOR 1 )
  set ( CPACK_PACKAGE_VERSION_PATCH ${Framework_WC_REVISION} )
  set ( CPACK_PACKAGING_INSTALL_PREFIX /opt/${CMAKE_PROJECT_NAME} )

  # RPM information - only used if generating a rpm
  #set ( CPACK_RPM_PACKAGE_ARCHITECTURE x86_64 )
  set ( CPACK_RPM_PACKAGE_LICENSE GPLv3+ )
  set ( CPACK_RPM_PACKAGE_RELEASE 1 )
  set ( CPACK_RPM_PACKAGE_GROUP Applications/Engineering )


  # DEB information - only used if generating a deb
  set ( CPACK_DEBIAN_PACKAGE_RELEASE 1 )

  # rhel requirements
  set ( CPACK_RPM_PACKAGE_REQUIRES "boost >= 1.34.1,qt4 >= 4.2,nexus,qscintilla,qwt,gsl,glibc,qwtplot3d-qt4,OpenCASCADE-libs-modelling >= 6.3.0,OpenCASCADE-libs-foundation >= 6.3.0,OpenCASCADE-libs-visualization >= 6.3.0,OpenCASCADE-libs-ocaf >= 6.3.0,OpenCASCADE-libs-ocaf-lite >= 6.3.0,muParser,numpy" )
  set ( CPACK_RPM_PACKAGE_REQUIRES "${CPACK_RPM_PACKAGE_REQUIRES},poco-crypto,poco-data,poco-mysql,poco-sqlite,poco-odbc,poco-util,poco-xml,poco-zip,poco-net,poco-netssl,poco-foundation" )
  IF ( ${CPACK_RPM_ARCHITECTURE} MATCHES "i386" )
    set ( CPACK_RPM_PACKAGE_REQUIRES "${CPACK_RPM_PACKAGE_REQUIRES},google-perftools" ) # doesn't exist on 64 bit RHEL
  ENDIF ( ${CPACK_RPM_ARCHITECTURE} MATCHES "i386" )

  # TODO get this right <----------------------
  set ( CPACK_DEBIAN_PACKAGE_DEPENDS "boost >= 1.34.1,qt4 >= 4.2,nexus,qscintilla,qwt,gsl,glibc,google-perftools,qwtplot3d-qt4,OpenCASCADE-libs-modelling >= 6.3.0,OpenCASCADE-libs-foundation >= 6.3.0,OpenCASCADE-libs-visualization >= 6.3.0,OpenCASCADE-libs-ocaf >= 6.3.0,OpenCASCADE-libs-ocaf-lite >= 6.3.0,muParser,numpy,poco" )

  # let people know what is coming out the other end
  message ( status " CPACK_PACKAGE_FILE_NAME = ${CPACK_PACKAGE_FILE_NAME}" )

  # run cpack configuration
  include ( CPack )
endif ()
