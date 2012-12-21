include_directories (libgit2/include)
add_subdirectory (libgit2)

set ( SRC_FILES src/GitScriptRepository.cpp
)

set ( INC_FILES inc/MantidScriptRepository/GitScriptRepository.h
)

set ( TEST_FILES
	GitScriptRepositoryTest.h
  GitConnectionTest.h
  GitNewFeatureTest.h
  ScriptRepositoryFactoryTest.h
)

# Add the target for this directory
add_library ( ScriptRepository ${SRC_FILES} ${INC_FILES})
# Add the unit tests directory
add_subdirectory ( test )
# Set the name of the generated library
set_target_properties ( ScriptRepository PROPERTIES OUTPUT_NAME MantidScriptRepository )

set_property (TARGET ScriptRepository PROPERTY FOLDER "MantidFramework")

include_directories(inc)
target_link_libraries(ScriptRepository ${MANTIDLIBS} git2 PocoZip)

install (TARGETS ScriptRepository ${SYSTEM_PACKAGE_TARGET} DESTINATION ${LIB_DIR} )