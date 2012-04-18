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

	#Windows CPACK specifics
	set( CPACK_GENERATOR "NSIS" )
	set( CPACK_SET_DESTDIR "ON") 
	set( CPACK_INSTALL_PREFIX "/")
	set( CPACK_PACKAGE_INSTALL_DIRECTORY "Mantid${CPACK_PACKAGE_SUFFIX}")
	set( CPACK_NSIS_INSTALL_ROOT "C:\\\\MantidInstall")

	#set( CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL "ON")
	set( CPACK_PACKAGE_ICON "${CMAKE_CURRENT_SOURCE_DIR}/Images\\\\MantidPlot_Icon_32offset.png" )
    set( CPACK_NSIS_MUI_ICON "${CMAKE_CURRENT_SOURCE_DIR}/Images\\\\MantidPlot_Icon_32offset.ico" )
    set( CPACK_NSIS_MUI_UNIICON "${CMAKE_CURRENT_SOURCE_DIR}/Images\\\\MantidPlot_Icon_32offset.ico" )
	set( WINDOWS_DEPLOYMENT_TYPE "Release" CACHE STRING "Type of deployment used")
	set_property(CACHE WINDOWS_DEPLOYMENT_TYPE PROPERTY STRINGS Release Debug)
	mark_as_advanced(WINDOWS_DEPLOYMENT_TYPE)
	
	#Manually place necessary files and directories
	
	#python bundle here.
	install ( DIRECTORY ${CMAKE_LIBRARY_PATH}/Python27/DLLs DESTINATION bin PATTERN ".svn" EXCLUDE PATTERN ".git" EXCLUDE )
	install ( DIRECTORY ${CMAKE_LIBRARY_PATH}/Python27/Lib DESTINATION bin PATTERN ".svn" EXCLUDE PATTERN ".git" EXCLUDE )
	install ( DIRECTORY ${CMAKE_LIBRARY_PATH}/Python27/Scripts DESTINATION bin PATTERN ".svn" EXCLUDE PATTERN ".git" EXCLUDE )
	install ( FILES ${CMAKE_LIBRARY_PATH}/Python27/python.exe ${CMAKE_LIBRARY_PATH}/Python27/python27.dll DESTINATION bin )

	install ( DIRECTORY ${CMAKE_LIBRARY_PATH}/qt_plugins/imageformats DESTINATION plugins/qtplugins PATTERN ".svn" EXCLUDE PATTERN ".git" EXCLUDE )
	install ( FILES ${CMAKE_LIBRARY_PATH}/qt.conf DESTINATION bin )
	
	#Handle includes
	install ( DIRECTORY ${CMAKE_INCLUDE_PATH}/boost DESTINATION include PATTERN ".svn" EXCLUDE PATTERN ".git" EXCLUDE )
	install ( DIRECTORY ${CMAKE_INCLUDE_PATH}/Poco DESTINATION include PATTERN ".svn" EXCLUDE PATTERN ".git" EXCLUDE )
	install ( DIRECTORY ${CMAKE_INCLUDE_PATH}/nexus DESTINATION include PATTERN ".svn" EXCLUDE PATTERN ".git" EXCLUDE )
	install ( FILES ${CMAKE_INCLUDE_PATH}/napi.h DESTINATION include )
	install ( DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Framework/Kernel/inc/MantidKernel DESTINATION include PATTERN ".svn" EXCLUDE PATTERN ".git" EXCLUDE )
	install ( DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Framework/Geometry/inc/MantidGeometry DESTINATION include PATTERN ".svn" EXCLUDE PATTERN ".git" EXCLUDE )
	install ( DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Framework/API/inc/MantidAPI DESTINATION include PATTERN ".svn" EXCLUDE PATTERN ".git" EXCLUDE )
	install ( DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Framework/NexusCPP/inc/MantidNexusCPP DESTINATION include PATTERN ".svn" EXCLUDE PATTERN ".git" EXCLUDE )
	
	#Copy scons directory
	install ( DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Installers/WinInstaller/scons-local/ DESTINATION scons-local PATTERN ".svn" EXCLUDE PATTERN ".git" EXCLUDE )
	#User algorithms
	install ( DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Framework/UserAlgorithms/ DESTINATION UserAlgorithms FILES_MATCHING PATTERN "*.h" )
	install ( DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Framework/UserAlgorithms/ DESTINATION UserAlgorithms FILES_MATCHING PATTERN "*.cpp" )
	install ( FILES ${CMAKE_CURRENT_SOURCE_DIR}/Framework/UserAlgorithms/build.bat ${CMAKE_CURRENT_SOURCE_DIR}/Framework/UserAlgorithms/createAlg.py ${CMAKE_CURRENT_SOURCE_DIR}/Framework/UserAlgorithms/SConstruct DESTINATION UserAlgorithms )
	install ( FILES "${CMAKE_CURRENT_BINARY_DIR}/bin/${WINDOWS_DEPLOYMENT_TYPE}/MantidKernel.lib" DESTINATION UserAlgorithms)
	install ( FILES "${CMAKE_CURRENT_BINARY_DIR}/bin/${WINDOWS_DEPLOYMENT_TYPE}/MantidGeometry.lib" DESTINATION UserAlgorithms)
	install ( FILES "${CMAKE_CURRENT_BINARY_DIR}/bin/${WINDOWS_DEPLOYMENT_TYPE}/MantidAPI.lib" DESTINATION UserAlgorithms)
	install ( FILES "${CMAKE_CURRENT_BINARY_DIR}/bin/${WINDOWS_DEPLOYMENT_TYPE}/MantidNexusCPP.lib" DESTINATION UserAlgorithms)
	install ( FILES "${CMAKE_CURRENT_BINARY_DIR}/bin/${WINDOWS_DEPLOYMENT_TYPE}/MantidDataObjects.lib" DESTINATION UserAlgorithms)
	install ( FILES "${CMAKE_CURRENT_BINARY_DIR}/bin/${WINDOWS_DEPLOYMENT_TYPE}/MantidCurveFitting.lib" DESTINATION UserAlgorithms)
	install ( FILES ${CMAKE_LIBRARY_PATH}/PocoFoundation.lib ${CMAKE_LIBRARY_PATH}/PocoXML.lib ${CMAKE_LIBRARY_PATH}/boost_date_time-vc100-mt-1_43.lib DESTINATION UserAlgorithms)
	
	# Copy third party dlls excluding selected Qt ones and debug ones
	install ( DIRECTORY ${CMAKE_LIBRARY_PATH}/ DESTINATION bin FILES_MATCHING PATTERN "*.dll" REGEX "(QtDesigner4.dll)|(QtDesignerComponents4.dll)|(QtScript4.dll)|(-gd-)|(d4.dll)|(_d.dll)" EXCLUDE PATTERN ".git" EXCLUDE )
	
	#set(CPACK_NSIS_ON_INIT  "Exec $INSTDIR\\\\Uninstall.exe")
	
	#Release deployments do modify enviromental varialbes, other deployments do not.
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
            Push \\\"$INSTDIR\\\\${PVPLUGINS_DIR}\\\"
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
            Push \\\"$INSTDIR\\\\${PVPLUGINS_DIR}\\\"
            Call un.EnvVarUpdate
            Pop  \\\$0
		
		    Delete \\\"$DESKTOP\\\\MantidPlot.lnk\\\"
		
		    RMDir \\\"$INSTDIR\\\\logs\\\"
		
		    RMDir \\\"$INSTDIR\\\\docs\\\"
	    ")
	else ()
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