###########################################################################
# CMake version check.
# Windows requires 2.8.3 with a fix with the FindZLib module
# everywhere else anything from the 2.8 series will do
###########################################################################
if ( WIN32 )
  cmake_minimum_required ( VERSION 2.8.3 )
else()
  cmake_minimum_required ( VERSION 2.8 )
endif ()

# System package target is important for the windows builds as it allows us to package only the dlls and exes and exclude libs. Defaults to empty for other platforms.
set ( SYSTEM_PACKAGE_TARGET "")

# Add the path to our custom 'find' modules
set ( CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/Build/CMake")

# Define the project name.
project ( Mantid )
set ( CPACK_PACKAGE_SUFFIX "" CACHE STRING "suffix used to determine the deployment type")
set_property(CACHE CPACK_PACKAGE_SUFFIX PROPERTY STRINGS nightly unstable "") #empty string and release are treated as the same thing

#Set package name here 
set ( CPACK_PACKAGE_NAME "mantid${CPACK_PACKAGE_SUFFIX}" )

###########################################################################
# Check for Qt (DarwinSetup below needs a variable from this)
###########################################################################
find_package ( Qt4 COMPONENTS QtCore QtGui QtOpenGL QtXml QtSvg Qt3Support REQUIRED )
include ( UseSystemQt4 )

###########################################################################
# Set paths to Third_Party for Windows builds
###########################################################################
IF(${CMAKE_SYSTEM_NAME} MATCHES "Windows")	

	if ( NOT THIRD_PARTY )
  		set ( THIRD_PARTY "${PROJECT_SOURCE_DIR}/../Third_Party" )  
	endif ()
	# Print out were we think we are looking for 3rd party stuff
	message (STATUS "Setting THIRD_PARTY to be ${THIRD_PARTY}." )

	# Check that the 3rd party directory exists.
	if (NOT IS_DIRECTORY "${THIRD_PARTY}")
        message ( WARNING "Specified THIRD_PARTY directory doesn't exist!" )
	endif()
	
	include ( WindowsSetup )
	
	# Settings for generating the windows Nullsoft package via CPack
	include ( WindowsNSIS )	
	
ENDIF()

###########################################################################
# Set paths to Third_Party for Mac builds
###########################################################################
IF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")	

execute_process(
      COMMAND /usr/bin/sw_vers -productVersion
      OUTPUT_VARIABLE OSX_VERSION
      RESULT_VARIABLE OSX_VERSION_STATUS
  )
  
  # Strip off any /CR or /LF
  string(STRIP ${OSX_VERSION} OSX_VERSION)

  if (OSX_VERSION VERSION_LESS 10.6)
    message (FATAL_ERROR "The minimum supported version of Mac OS X is 10.6 (Snow Leopard).")
  endif()

  if (OSX_VERSION VERSION_GREATER 10.6 OR OSX_VERSION VERSION_EQUAL 10.6)
    set ( OSX_CODENAME "Snow Leopard" )
  endif()

  if (OSX_VERSION VERSION_GREATER 10.7 OR OSX_VERSION VERSION_EQUAL 10.7)
    set ( OSX_CODENAME "Lion")
  endif()

  if (OSX_VERSION VERSION_GREATER 10.8 OR OSX_VERSION VERSION_EQUAL 10.8)
    set ( OSX_CODENAME "Mountain Lion")
  endif()

  message (STATUS "Operating System: Mac OS X ${OSX_VERSION} (${OSX_CODENAME})")

	if ( NOT THIRD_PARTY )
  		set ( THIRD_PARTY "${PROJECT_SOURCE_DIR}/../Third_Party" )  
	endif ()

	# Print out were we think we are looking for 3rd party stuff
	message (STATUS "Setting THIRD_PARTY to be ${THIRD_PARTY}." )

	# Check that the 3rd party directory exists.
	if (NOT IS_DIRECTORY "${THIRD_PARTY}")
        message ( WARNING "Specified THIRD_PARTY directory doesn't exist!" )
	endif()

    include ( DarwinSetup )	
    
ENDIF()

###########################################################################
# Call our setup script
###########################################################################
include ( CommonSetup )

###########################################################################
# Find qwt. Needs to be version 5
###########################################################################
find_package ( Qwt REQUIRED )
if( QWT_VERSION VERSION_LESS 5.0 OR QWT_VERSION VERSION_EQUAL 6.0 OR 
    QWT_VERSION VERSION_GREATER 6.0 )
  message ( FATAL_ERROR "Qwt version 5 is required, found: ${QWT_VERSION}" )
endif() 

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
include_directories ( Framework/NexusCPP/inc )
set ( CORE_MANTIDLIBS Kernel Geometry API )

# Add a target for all GUI tests
add_custom_target ( GUITests ) 
add_dependencies ( check GUITests )

add_subdirectory ( MantidQt )

include_directories ( QtPropertyBrowser/src )
include_directories ( MantidQt/API/inc )
include_directories ( MantidQt/MantidWidgets/inc )
include_directories ( ${CMAKE_CURRENT_BINARY_DIR}/MantidQt/API )
include_directories ( ${CMAKE_CURRENT_BINARY_DIR}/MantidQt/MantidWidgets )
include_directories ( ${CMAKE_CURRENT_BINARY_DIR}/MantidQt/SliceViewer )

if ( UNIX )
  #Experimental feature. Unix only at this point.
  set ( UNITY_BUILD OFF CACHE BOOL "Switch for utilising unity builds. Faster builds for selected components.")
endif ( UNIX )

set ( MAKE_VATES OFF CACHE BOOL "Switch for compiling the Vates project")
set ( CXXTEST_SINGLE_LOGFILE CACHE BOOL "Switch to have the tests for each package run together")
set ( CXXTEST_ADD_PERFORMANCE OFF CACHE BOOL "Switch to add Performance tests to the list of tests run by ctest?") 

if ( MAKE_VATES )
  add_subdirectory ( Vates )
endif ( MAKE_VATES )

add_subdirectory ( MantidPlot )

# python unit tests
if (PYUNITTEST_FOUND)
  add_subdirectory ( scripts )
endif ()

# Collect all tests together
add_custom_target ( AllTests ) 
add_dependencies ( AllTests FrameworkTests )
add_dependencies ( AllTests GUITests )


###########################################################################
# Installation settings
###########################################################################

# N.B. INBUNDLE variable is empty except on Mac (set in DarwinSetup.cmake)
install ( DIRECTORY Installers/colormaps/ DESTINATION ${INBUNDLE}colormaps PATTERN ".svn" EXCLUDE )

# Install the files (.desktop and icon) to create a menu item, but only if installing to /opt/Mantid
if ( ${CMAKE_SYSTEM_NAME} STREQUAL "Linux" AND CMAKE_INSTALL_PREFIX STREQUAL "/opt/Mantid" )
  install ( FILES ${CMAKE_CURRENT_SOURCE_DIR}/Installers/LinuxInstaller/mantidplot.desktop
            DESTINATION /usr/share/applications )
  install ( FILES ${CMAKE_CURRENT_SOURCE_DIR}/Images/MantidPlot_Icon_32offset.png
            DESTINATION /usr/share/pixmaps
            RENAME mantidplot.png )
endif()

###########################################################################
# CPack settings
###########################################################################

# Keep this in a defaulted-to-off option
set ( ENABLE_CPACK CACHE BOOL "Switch to enable CPack package generation")
if ( ENABLE_CPACK )
  #if(NOT ${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
  
    include ( CPackCommon )
    
    #set ( CPACK_PACKAGE_ICON Images/MantidPlot_Icon_32offset.png )
    # Mac Packagemaker settings
    set ( CPACK_RESOURCE_FILE_README ${CMAKE_SOURCE_DIR}/Installers/MacInstaller/MacOSX-README.rtf )
    set ( CPACK_RESOURCE_FILE_LICENSE ${CMAKE_SOURCE_DIR}/Installers/WinInstaller/License.rtf )

    IF ( ${CMAKE_SYSTEM_NAME} STREQUAL "Linux" )
      include ( CPackLinuxSetup )    
      # let people know what is coming out the other end
      message ( STATUS " CPACK_PACKAGE_FILE_NAME = ${CPACK_PACKAGE_FILE_NAME}" )

      # rhel requirements
      set ( CPACK_RPM_PACKAGE_REQUIRES "boost >= 1.34.1,qt4 >= 4.2,nexus,qwt,gsl,glibc,qwtplot3d-qt4,OpenCASCADE-libs-modelling >= 6.3.0,OpenCASCADE-libs-foundation >= 6.3.0,OpenCASCADE-libs-visualization >= 6.3.0,OpenCASCADE-libs-ocaf >= 6.3.0,OpenCASCADE-libs-ocaf-lite >= 6.3.0,muParser,numpy" )
      set ( CPACK_RPM_PACKAGE_REQUIRES "${CPACK_RPM_PACKAGE_REQUIRES},poco-crypto,poco-data,poco-mysql,poco-sqlite,poco-odbc,poco-util,poco-xml,poco-zip,poco-net,poco-netssl,poco-foundation,PyQt4,sip" )
      # On RHEL6 we have to use an updated qscintilla to fix an auto complete bug
      if( ${UNIX_CODENAME} STREQUAL "Santiago" )
        set ( CPACK_RPM_PACKAGE_REQUIRES "${CPACK_RPM_PACKAGE_REQUIRES} qscintilla >= 2.4.6" )
      else()  
        set ( CPACK_RPM_PACKAGE_REQUIRES "${CPACK_RPM_PACKAGE_REQUIRES} qscintilla" )
      endif()
      if( ${UNIX_DIST} STREQUAL "Ubuntu" )
        if( ${UNIX_CODENAME} STREQUAL "lucid" )
          set ( CPACK_DEBIAN_PACKAGE_DEPENDS "libboost-date-time1.40.0,libboost-regex1.40.0,libboost-signals1.40.0,libpocofoundation9,libpocoutil9,libpoconet9,libpoconetssl9,libpococrypto9,libpocoxml9,libnexus0 (>= 4.2),libgsl0ldbl,libqtcore4 (>= 4.2),libqtgui4 (>= 4.2),libqt4-opengl (>= 4.2),libqt4-xml (>= 4.2),libqt4-svg (>= 4.2),libqt4-qt3support (>= 4.2),libqscintilla2-5,libqwt5-qt4,libqwtplot3d-qt4-0" )
          set ( CPACK_DEBIAN_PACKAGE_DEPENDS "${CPACK_DEBIAN_PACKAGE_DEPENDS},libopencascade-foundation-6.3.0 (>= 6.3.0),libopencascade-modeling-6.3.0 (>= 6.3.0),libmuparser0,python-numpy,python-sip,python-qt4,libboost-python${Boost_MAJOR_VERSION}.${Boost_MINOR_VERSION}.${Boost_SUBMINOR_VERSION}" )
        elseif( ${UNIX_CODENAME} STREQUAL "precise" )
          set ( CPACK_DEBIAN_PACKAGE_DEPENDS "libboost-date-time1.46.1,libboost-regex1.46.1,libboost-signals1.46.1,libpocofoundation9,libpocoutil9,libpoconet9,libpoconetssl9,libpococrypto9,libpocoxml9,libnexus0 (>= 4.2),libgsl0ldbl,libqtcore4 (>= 4.2),libqtgui4 (>= 4.2),libqt4-opengl (>= 4.2),libqt4-xml (>= 4.2),libqt4-svg (>= 4.2),libqt4-qt3support (>= 4.2),libqscintilla2-8,libqwt5-qt4,libqwtplot3d-qt4-0" )
          set ( CPACK_DEBIAN_PACKAGE_DEPENDS "${CPACK_DEBIAN_PACKAGE_DEPENDS},libopencascade-foundation-6.5.0 (>= 6.5.0),libopencascade-modeling-6.5.0 (>= 6.5.0),libmuparser0debian1,python-numpy,python-sip,python-qt4,libboost-python${Boost_MAJOR_VERSION}.${Boost_MINOR_VERSION}.${Boost_SUBMINOR_VERSION}" )
        else()
          message( WARNING "Mantid does not support packaging of this Ubuntu version: ${UNIX_CODENAME}")
        endif()
      endif()
      # soft requirement of tcmalloc - if built with it, then require it
      IF ( USE_TCMALLOC AND TCMALLOC_FOUND )
        message ( STATUS " Adding gperftools to the package requirements" )
        set ( CPACK_RPM_PACKAGE_REQUIRES "${CPACK_RPM_PACKAGE_REQUIRES},gperftools-libs >= 2.0" )
        set ( CPACK_DEBIAN_PACKAGE_DEPENDS "${CPACK_DEBIAN_PACKAGE_DEPENDS},libgoogle-perftools0 (>= 1.7)" )
      ENDIF ( )
    ENDIF ()

    # run cpack configuration
    include ( CPack )
  #ENDIF(NOT ${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
endif ()
