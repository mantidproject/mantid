# Mantid source
set ( SRC_FILES 
  src/ScriptRepositoryImpl.cpp
)

set ( INC_FILES inc/MantidScriptRepository/ScriptRepositoryImpl.h
)

#
set ( TEST_FILES
	ScriptRepositoryTestImpl.h)
#        ScriptRepositoryFactoryTest.h)


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
target_link_libraries(ScriptRepository ${LIBS})
if ( MSVC )
target_link_libraries(ScriptRepository Winhttp)
endif()
install (TARGETS ScriptRepository ${SYSTEM_PACKAGE_TARGET} DESTINATION ${PLUGINS_DIR} )
