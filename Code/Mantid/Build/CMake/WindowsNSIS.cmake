##########################################################################
# Does the CPack configuration for Windows/NSIS
#
# Bundles python
# Copies include directories
# Copies scons
# Copies files required for User Algorithms
# Copies selected third party dlls accross
# Sets up env variables, shortcuts and required folders post install and post uninstall.
###########################################################################

    # Windows CPACK specifics
    set( CPACK_GENERATOR "NSIS" )
    set( CPACK_INSTALL_PREFIX "/")
    set( CPACK_NSIS_DISPLAY_NAME "Mantid${CPACK_PACKAGE_SUFFIX}")
    set( CPACK_PACKAGE_NAME "mantid${CPACK_PACKAGE_SUFFIX}" )
    set( CPACK_PACKAGE_INSTALL_DIRECTORY "MantidInstall${CPACK_PACKAGE_SUFFIX}") 
    set( CPACK_NSIS_INSTALL_ROOT "C:")
    set( CPACK_PACKAGE_EXECUTABLES "MantidPlot;MantidPlot")
    set( CPACK_NSIS_MENU_LINKS "bin\\\\MantidPlot.exe" "MantidPlot")
    
    set( CPACK_PACKAGE_ICON "${CMAKE_CURRENT_SOURCE_DIR}/Images\\\\MantidPlot_Icon_32offset.png" )
    set( CPACK_NSIS_MUI_ICON "${CMAKE_CURRENT_SOURCE_DIR}/Images\\\\MantidPlot_Icon_32offset.ico" )
    set( CPACK_NSIS_MUI_UNIICON "${CMAKE_CURRENT_SOURCE_DIR}/Images\\\\MantidPlot_Icon_32offset.ico" )
    set( WINDOWS_DEPLOYMENT_TYPE "Release" CACHE STRING "Type of deployment used")
    set_property(CACHE WINDOWS_DEPLOYMENT_TYPE PROPERTY STRINGS Release Debug)
    mark_as_advanced(WINDOWS_DEPLOYMENT_TYPE)
    
    # Manually place necessary files and directories
    
    # include files
    install ( DIRECTORY ${CMAKE_INCLUDE_PATH}/boost DESTINATION include PATTERN ".svn" EXCLUDE PATTERN ".git" EXCLUDE )
    install ( DIRECTORY ${CMAKE_INCLUDE_PATH}/Poco DESTINATION include PATTERN ".svn" EXCLUDE PATTERN ".git" EXCLUDE )
    install ( DIRECTORY ${CMAKE_INCLUDE_PATH}/nexus DESTINATION include PATTERN ".svn" EXCLUDE PATTERN ".git" EXCLUDE )
    install ( FILES ${CMAKE_INCLUDE_PATH}/napi.h DESTINATION include )
    install ( DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Framework/Kernel/inc/MantidKernel DESTINATION include PATTERN ".svn" EXCLUDE PATTERN ".git" EXCLUDE )
    install ( DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Framework/Geometry/inc/MantidGeometry DESTINATION include PATTERN ".svn" EXCLUDE PATTERN ".git" EXCLUDE )
    install ( DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Framework/API/inc/MantidAPI DESTINATION include PATTERN ".svn" EXCLUDE PATTERN ".git" EXCLUDE )
    
    # scons directory for sser building
    install ( DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Installers/WinInstaller/scons-local/ DESTINATION scons-local PATTERN ".svn" EXCLUDE PATTERN ".git" EXCLUDE )
    # user algorithms
    install ( DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Framework/UserAlgorithms/ DESTINATION UserAlgorithms FILES_MATCHING PATTERN "*.h" )
    install ( DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Framework/UserAlgorithms/ DESTINATION UserAlgorithms FILES_MATCHING PATTERN "*.cpp" )
    install ( FILES ${CMAKE_CURRENT_SOURCE_DIR}/Framework/UserAlgorithms/build.bat ${CMAKE_CURRENT_SOURCE_DIR}/Framework/UserAlgorithms/createAlg.py 
              ${CMAKE_CURRENT_SOURCE_DIR}/Framework/UserAlgorithms/SConstruct DESTINATION UserAlgorithms )
    install ( FILES "${CMAKE_BINARY_DIR}/bin/${WINDOWS_DEPLOYMENT_TYPE}/MantidKernel.lib" DESTINATION UserAlgorithms)
    install ( FILES "${CMAKE_BINARY_DIR}/bin/${WINDOWS_DEPLOYMENT_TYPE}/MantidGeometry.lib" DESTINATION UserAlgorithms)
    install ( FILES "${CMAKE_BINARY_DIR}/bin/${WINDOWS_DEPLOYMENT_TYPE}/MantidAPI.lib" DESTINATION UserAlgorithms)
    install ( FILES "${CMAKE_BINARY_DIR}/bin/${WINDOWS_DEPLOYMENT_TYPE}/MantidDataObjects.lib" DESTINATION UserAlgorithms)
    install ( FILES "${CMAKE_BINARY_DIR}/bin/${WINDOWS_DEPLOYMENT_TYPE}/MantidCurveFitting.lib" DESTINATION UserAlgorithms)
    # Poco libs for UserAlgorithms. Boost name depends on compiler version
    install ( FILES ${CMAKE_LIBRARY_PATH}/PocoFoundation.lib ${CMAKE_LIBRARY_PATH}/PocoXML.lib DESTINATION UserAlgorithms)
    
    # Copy MSVC runtime libraries
    if ( MSVC_VERSION EQUAL 1700 )
      set ( RUNTIME_VER 110 )
      # Boost library is a different version
      install ( FILES ${CMAKE_LIBRARY_PATH}/boost_date_time-vc110-mt-1_52.lib DESTINATION UserAlgorithms )
    else() # Assume 100 like we always did
      set ( RUNTIME_VER 100 )
      # Boost library is a different version
      install ( FILES ${CMAKE_LIBRARY_PATH}/boost_date_time-vc100-mt-1_43.lib DESTINATION UserAlgorithms )
    endif ()

    file ( TO_CMAKE_PATH $ENV{VS${RUNTIME_VER}COMNTOOLS}/../../VC/redist VC_REDIST )
    if ( CMAKE_CL_64 )
        set ( VC_REDIST ${VC_REDIST}/x64 )
    else()
        set ( VC_REDIST ${VC_REDIST}/x86 )
    endif()
    # Runtime libraries
    set ( RUNTIME_DLLS msvcp${RUNTIME_VER}.dll msvcr${RUNTIME_VER}.dll )
    set ( REDIST_SUBDIR Microsoft.VC${RUNTIME_VER}.CRT )
    foreach( DLL ${RUNTIME_DLLS} )
        install ( FILES ${VC_REDIST}/${REDIST_SUBDIR}/${DLL} DESTINATION bin )
    endforeach()

    # openmp library(s)
    set ( OPENMP_DLLS vcomp${RUNTIME_VER}.dll )
    set ( REDIST_SUBDIR Microsoft.VC${RUNTIME_VER}.OpenMP )
    foreach( DLL ${OPENMP_DLLS} )
        install ( FILES ${VC_REDIST}/${REDIST_SUBDIR}/${DLL} DESTINATION bin )
    endforeach() 

    # Copy third party dlls excluding selected Qt ones and debug ones
    install ( DIRECTORY ${CMAKE_LIBRARY_PATH}/ DESTINATION bin FILES_MATCHING PATTERN "*.dll"
    REGEX "${CMAKE_LIBRARY_PATH}/CRT/*" EXCLUDE
    REGEX "${CMAKE_LIBRARY_PATH}/Python27/*" EXCLUDE 
    REGEX "${CMAKE_LIBRARY_PATH}/qt_plugins/*" EXCLUDE 
    REGEX "(QtDesigner4.dll)|(QtDesignerComponents4.dll)|(QtScript4.dll)|(-gd-)|(-gyd)|(d4.dll)|(_d.dll)" EXCLUDE
    REGEX "boost_signals" EXCLUDE
    PATTERN ".git" EXCLUDE )

    # Qt plugins into out plugins directory and use qt.conf to point Qt at where they are
    install ( DIRECTORY ${CMAKE_LIBRARY_PATH}/qt_plugins/imageformats ${CMAKE_LIBRARY_PATH}/qt_plugins/sqldrivers DESTINATION plugins/qtplugins 
              REGEX "^.*d4.dll$" EXCLUDE  )
    install ( FILES ${CMAKE_CURRENT_SOURCE_DIR}/Installers/WinInstaller/qt.conf DESTINATION bin )
   
    # Release deployments do modify enviromental variables, other deployments do not.
    if(CPACK_PACKAGE_SUFFIX STREQUAL "") 
        # On install
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
            
            Push \\\"PATH\\\"
            Push \\\"A\\\"
            Push \\\"HKCU\\\"
            Push \\\"$INSTDIR\\\\${PVPLUGINS_DIR}\\\"
            Call EnvVarUpdate
            Pop  \\\$0
            
            Push \\\"PATH\\\"
            Push \\\"A\\\"
            Push \\\"HKCU\\\"
            Push \\\"$INSTDIR\\\\${PLUGINS_DIR}\\\"
            Call EnvVarUpdate
            Pop  \\\$0
            
            Push \\\"PV_PLUGIN_PATH\\\"
            Push \\\"A\\\"
            Push \\\"HKCU\\\"
            Push \\\"$INSTDIR\\\\${PVPLUGINS_DIR}\\\\${PVPLUGINS_DIR}\\\"
            Call EnvVarUpdate
            Pop  \\\$0
        
            CreateShortCut \\\"$DESKTOP\\\\MantidPlot.lnk\\\" \\\"$INSTDIR\\\\bin\\\\MantidPlot.exe\\\"
        
            CreateDirectory \\\"$INSTDIR\\\\logs\\\"
        
            CreateDirectory \\\"$INSTDIR\\\\docs\\\"
        ")
    # On unistall reverse stages listed above.
        set (CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS 
            "Push \\\"PATH\\\"
            Push \\\"R\\\"
            Push \\\"HKCU\\\"
            Push \\\"$INSTDIR\\\\bin\\\"
            Call un.EnvVarUpdate
            Pop  \\\$0
            
            Push \\\"PATH\\\"
            Push \\\"R\\\"
            Push \\\"HKCU\\\"
            Push \\\"$INSTDIR\\\\${PVPLUGINS_DIR}\\\"
            Call un.EnvVarUpdate
            Pop  \\\$0
            
            Push \\\"PATH\\\"
            Push \\\"R\\\"
            Push \\\"HKCU\\\"
            Push \\\"$INSTDIR\\\\${PLUGINS_DIR}\\\"
            Call un.EnvVarUpdate
            Pop  \\\$0
        
            Push \\\"MANTIDPATH\\\"
            Push \\\"R\\\"
            Push \\\"HKCU\\\"
            Push \\\"$INSTDIR\\\\bin\\\"
            Call un.EnvVarUpdate
            Pop  \\\$0

            Push \\\"PV_PLUGIN_PATH\\\"
            Push \\\"R\\\"
            Push \\\"HKCU\\\"
            Push \\\"$INSTDIR\\\\${PVPLUGINS_DIR}\\\\${PVPLUGINS_DIR}\\\"
            Call un.EnvVarUpdate
            Pop  \\\$0
        
            Delete \\\"$DESKTOP\\\\MantidPlot.lnk\\\"
        
            RMDir \\\"$INSTDIR\\\\logs\\\"
        
            RMDir \\\"$INSTDIR\\\\docs\\\"
        ")
    else ()
    set( CPACK_PACKAGE_INSTALL_DIRECTORY "MantidInstall${CPACK_PACKAGE_SUFFIX}")
    set( CPACK_NSIS_INSTALL_ROOT "C:")
    # On install
        set (CPACK_NSIS_EXTRA_INSTALL_COMMANDS "
            CreateShortCut \\\"$DESKTOP\\\\MantidPlot.lnk\\\" \\\"$INSTDIR\\\\bin\\\\MantidPlot.exe\\\"
        
            CreateDirectory \\\"$INSTDIR\\\\logs\\\"
        
            CreateDirectory \\\"$INSTDIR\\\\docs\\\"
        ")
    # On unistall reverse stages listed above.
        set (CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS "
            Delete \\\"$DESKTOP\\\\MantidPlot.lnk\\\"
        
            RMDir \\\"$INSTDIR\\\\logs\\\"
        
            RMDir \\\"$INSTDIR\\\\docs\\\"
        ")
    endif()
