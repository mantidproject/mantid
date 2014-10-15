# Mantid source
set ( SRC_FILES
	src/ScriptRepositoryImpl.cpp
)

set ( INC_FILES
	inc/MantidScriptRepository/ProxyInfo.h
	inc/MantidScriptRepository/ProxyOSX.h
	inc/MantidScriptRepository/ScriptRepositoryImpl.h
)

set ( TEST_FILES
	ScriptRepositoryTestImpl.h
)


set ( LIBS ${MANTIDLIBS} )
if ( ${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    LIST(APPEND SRC_FILES src/ProxyInfo.cpp src/ProxyOSX.cpp)
    LIST(APPEND INC_FILES inc/MantidScriptRepository/ProxyInfo.h inc/MantidScriptRepository/ProxyOSX.h)
    LIST(APPEND TEST_FILES ProxyInfoTest.h)
    LIST(APPEND LIBS "-framework SystemConfiguration" "-framework CoreFoundation"  "-framework CoreServices")
endif()

# Add the target for this directory
add_library ( ScriptRepository ${SRC_FILES} ${INC_FILES} )
# Add the unit tests directory
add_subdirectory ( test )
# Set the name of the generated library
set_target_properties ( ScriptRepository PROPERTIES OUTPUT_NAME MantidScriptRepository 
                        COMPILE_DEFINITIONS IN_MANTID_SCRIPTREPO)

set_property (TARGET ScriptRepository PROPERTY FOLDER "MantidFramework")

include_directories(inc)
set ( LIBS ${MANTIDLIBS} )
if ( ${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    LIST(APPEND LIBS "-framework SystemConfiguration" "-framework CoreFoundation"  "-framework CoreServices")
endif()

target_link_libraries(ScriptRepository ${LIBS})
if ( MSVC )
target_link_libraries(ScriptRepository Winhttp)
endif()
install (TARGETS ScriptRepository ${SYSTEM_PACKAGE_TARGET} DESTINATION ${PLUGINS_DIR} )
