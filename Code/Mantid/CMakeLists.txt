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


# Add the path to our custom 'find' modules
set ( CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/Build/CMake")

# Define the project name.
project ( Mantid )
set ( CPACK_PACKAGE_SUFFIX "" CACHE STRING "suffix used in linux packages and for symlinks")

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
	
	#Windows CPACK specifics
	set( CPACK_GENERATOR "NSIS" )
	set( CPACK_SET_DESTDIR "ON") 
	set( CPACK_INSTALL_PREFIX "/")
	set( CPACK_NSIS_INSTALL_ROOT "C:\\\\MantidInstall")
	set( TARGET_TYPE RUNTIME)
	
	#TODO: Tidy this up by excluding .git as well as .svn!
	#TODO check destinations are relative to the actual install location.
	
	#Manually place necessary files and directories
	
	#python bundle here.
	#install ( DIRECTORY ${CMAKE_LIBRARY_PATH}/Python27/DLLs DESTINATION bin/DLLs PATTERN ".svn" EXCLUDE )
	#install ( DIRECTORY ${CMAKE_LIBRARY_PATH}/Python27/Lib DESTINATION bin/Lib PATTERN ".svn" EXCLUDE )
	#install ( DIRECTORY ${CMAKE_LIBRARY_PATH}/Python27/Scripts DESTINATION bin/Scripts PATTERN ".svn" EXCLUDE )
	#install ( FILES ${CMAKE_LIBRARY_PATH}/Python27/python.exe ${CMAKE_LIBRARY_PATH}/Python27/python27.dll DESTINATION bin )

	#TODO: remove old pyc files on uninstall
	
	#install ( DIRECTORY ${CMAKE_LIBRARY_PATH}/qt_plugins/imageformats DESTINATION plugins/qtplugins PATTERN ".svn" EXCLUDE )
	#install ( FILES ${CMAKE_LIBRARY_PATH}/qt.conf DESTINATION bin )
	
	#TODO: create logs directory
	#TODO: create documents directory  what does the config service do if the logs directory is missing?
	
	#TODO: put these commands in the associated CMakeLists.
	
	#Handle includes
	#install ( DIRECTORY ${CMAKE_INCLUDE_PATH}/boost/ DESTINATION include PATTERN ".svn" EXCLUDE )
	#install ( DIRECTORY ${CMAKE_INCLUDE_PATH}/Poco/ DESTINATION include PATTERN ".svn" EXCLUDE )
	#install ( DIRECTORY ${CMAKE_INCLUDE_PATH}/nexus/ DESTINATION include PATTERN ".svn" EXCLUDE )
	#install ( FILES ${CMAKE_INCLUDE_PATH}/napi.h DESTINATION include )
	#install ( DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Framework/Kernel/inc/MantidKernel/ DESTINATION include PATTERN ".svn" EXCLUDE )
	#install ( DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Framework/Geometry/inc/MantidGeometry/ DESTINATION include PATTERN ".svn" EXCLUDE )
	#install ( DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Framework/API/inc/MantidAPI/ DESTINATION include PATTERN ".svn" EXCLUDE )
	#install ( DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Framework/NexusCPP/inc/MantidNexusCPP/ DESTINATION include PATTERN ".svn" EXCLUDE )
	
	#Copy scons directory
	install ( DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Installers/WinInstaller/scons-local/ DESTINATION scons-local PATTERN ".svn" EXCLUDE )
	#User algorithms
	#install ( DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Framework/UserAlgorithms/ DESTINATION UserAlgorithms FILES_MATCHING PATTERN "*.h" )
	#install ( DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Framework/UserAlgorithms/ DESTINATION UserAlgorithms FILES_MATCHING PATTERN "*.cpp" )
	#install ( FILES ${CMAKE_CURRENT_SOURCE_DIR}/Framework/UserAlgorithms/build.bat ${CMAKE_CURRENT_SOURCE_DIR}/Framework/UserAlgorithms/createAlg.py ${CMAKE_CURRENT_SOURCE_DIR}/Framework/UserAlgorithms/SConstruct DESTINATION UserAlgorithms )
	#install ( FILES "${CMAKE_CURRENT_BINARY_DIR}/bin/Debug/MantidKernel.lib" DESTINATION UserAlgorithms)
	#install ( FILES "${CMAKE_CURRENT_BINARY_DIR}/bin/Debug/MantidGeometry.lib" DESTINATION UserAlgorithms)
	#install ( FILES "${CMAKE_CURRENT_BINARY_DIR}/bin/Debug/MantidAPI.lib" DESTINATION UserAlgorithms)
	#install ( FILES "${CMAKE_CURRENT_BINARY_DIR}/bin/Debug/MantidNexusCPP.lib" DESTINATION UserAlgorithms)
	#install ( FILES "${CMAKE_CURRENT_BINARY_DIR}/bin/Debug/MantidDataObjects.lib" DESTINATION UserAlgorithms)
	#install ( FILES "${CMAKE_CURRENT_BINARY_DIR}/bin/Debug/MantidCurveFitting.lib" DESTINATION UserAlgorithms)
	#install ( FILES ${CMAKE_LIBRARY_PATH}/PocoFoundation.lib ${CMAKE_LIBRARY_PATH}/PocoXML.lib ${CMAKE_LIBRARY_PATH}/boost_date_time-vc100-mt-1_43.lib DESTINATION UserAlgorithms)
	
	
	# On install
	#TODO only set env variables for the release builds.
	set (CPACK_NSIS_EXTRA_INSTALL_COMMANDS "Push \\\"MANTIDPATH\\\"
        Push \\\"A\\\"
        Push \\\"HKCU\\\"
        Push \\\"$INSTDIR\\\\bin\\\"
        Call EnvVarUpdate
        Pop  \\\$0
		
	    Push \\\"PATH\\\"
        Push \\\"A\\\"
        Push \\\"HKCU\\\"
        Push \\\"$INSTDIR\\\\bin\\\"
        Call EnvVarUpdate
        Pop  \\\$0
		
		CreateShortCut \\\"$DESKTOP\\\\MantidPlot.lnk\\\" \\\"$INSTDIR\\\\bin\\\\MantidPlot.exe\\\"
	")
		
	# On unistall reverse stages listed above.
	set (CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS 
        "Push \\\"PATH\\\"
        Push \\\"R\\\"
        Push \\\"HKCU\\\"
        Push \\\"$INSTDIR\\\\bin\\\"
        Call un.EnvVarUpdate
        Pop  \\\$0
		
		Push \\\"MANTIDPATH\\\"
		Push \\\"R\\\"
        Push \\\"HKCU\\\"
        Push \\\"$INSTDIR\\\\bin\\\"
        Call un.EnvVarUpdate
        Pop  \\\$0
		
		Delete \\\"$DESKTOP\\\\MantidPlot.lnk\\\"
	")
	  
ENDIF()

###########################################################################
# Set paths to Third_Party for Mac builds
###########################################################################
IF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")	

  execute_process(
      COMMAND /usr/bin/sw_vers -product
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

add_subdirectory ( MantidPlot )

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
    set ( CPACK_PACKAGE_NAME "mantid${CPACK_PACKAGE_SUFFIX}" )
  
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

      set ( CPACK_DEBIAN_PACKAGE_DEPENDS "libboost-date-time1.40.0,libboost-regex1.40.0,libboost-signals1.40.0,libpocofoundation9,libpocoutil9,libpoconet9,libpoconetssl9,libpococrypto9,libpocoxml9,libnexus0 (>= 4.2),libgsl0ldbl,libqtcore4 (>= 4.2),libqtgui4 (>= 4.2),libqt4-opengl (>= 4.2),libqt4-xml (>= 4.2),libqt4-svg (>= 4.2),libqt4-qt3support (>= 4.2),libqscintilla2-5,libqwt5-qt4,libqwtplot3d-qt4-0" )
      set ( CPACK_DEBIAN_PACKAGE_DEPENDS "${CPACK_DEBIAN_PACKAGE_DEPENDS},libopencascade-foundation-6.3.0 (>= 6.3.0),libopencascade-modeling-6.3.0 (>= 6.3.0),libmuparser0,python-numpy,python-sip,python-qt4,libboost-python${Boost_MAJOR_VERSION}.${Boost_MINOR_VERSION}.${Boost_SUBMINOR_VERSION}" )

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
