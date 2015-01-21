# Mantid source
set ( SRC_FILES
	src/ScriptRepositoryImpl.cpp
)

set ( INC_FILES
	inc/MantidScriptRepository/ScriptRepositoryImpl.h
)

set ( TEST_FILES
	ScriptRepositoryTestImpl.h
)


# Add the target for this directory
add_library ( ScriptRepository ${SRC_FILES} ${INC_FILES} )
# Add the unit tests directory
add_subdirectory ( test )
# Set the name of the generated library
set_target_properties ( ScriptRepository PROPERTIES OUTPUT_NAME MantidScriptRepository 
                        COMPILE_DEFINITIONS IN_MANTID_SCRIPTREPO )

if (OSX_VERSION VERSION_GREATER 10.8)
  set_target_properties ( ScriptRepository PROPERTIES INSTALL_RPATH "@loader_path/../Contents/MacOS")
endif ()

set_property (TARGET ScriptRepository PROPERTY FOLDER "MantidFramework")

set ( LIBS ${MANTIDLIBS} )

include_directories(inc)

target_link_libraries(ScriptRepository ${LIBS})

install (TARGETS ScriptRepository ${SYSTEM_PACKAGE_TARGET} DESTINATION ${PLUGINS_DIR} )
