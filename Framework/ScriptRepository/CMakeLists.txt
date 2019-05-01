# Mantid source
set(SRC_FILES
    src/ScriptRepositoryImpl.cpp)

set(INC_FILES
    inc/MantidScriptRepository/DllConfig.h
    inc/MantidScriptRepository/ScriptRepositoryImpl.h)

set(TEST_FILES
    ScriptRepositoryTestImpl.h)

if(COVERALLS)
  foreach(loop_var ${SRCS_FILES} ${INC_FILES})
    set_property(GLOBAL APPEND
                 PROPERTY COVERAGE_SRCS
                          "${CMAKE_CURRENT_SOURCE_DIR}/${loop_var}")
  endforeach(loop_var)
endif()

# Add the target for this directory
add_library(ScriptRepository ${SRC_FILES} ${INC_FILES})
# Add the unit tests directory
add_subdirectory(test)
# Set the name of the generated library
set_target_properties(ScriptRepository
                      PROPERTIES OUTPUT_NAME
                                 MantidScriptRepository
                                 COMPILE_DEFINITIONS
                                 IN_MANTID_SCRIPTREPO)
if(OSX_VERSION VERSION_GREATER 10.8)
  set_target_properties(ScriptRepository
                        PROPERTIES INSTALL_RPATH
                                   "@loader_path/../Contents/MacOS")
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  set_target_properties(ScriptRepository
                        PROPERTIES INSTALL_RPATH "\$ORIGIN/../${LIB_DIR}")
endif()

set_property(TARGET ScriptRepository PROPERTY FOLDER "MantidFramework")

set(LIBS ${MANTIDLIBS})

include_directories(inc)

target_link_libraries(ScriptRepository
                      LINK_PRIVATE
                      ${TCMALLOC_LIBRARIES_LINKTIME}
                      ${LIBS}
                      ${JSONCPP_LIBRARIES})

mtd_install_targets(TARGETS
                    ScriptRepository
                    INSTALL_DIRS
                    ${PLUGINS_DIR}
                    ${WORKBENCH_PLUGINS_DIR})
