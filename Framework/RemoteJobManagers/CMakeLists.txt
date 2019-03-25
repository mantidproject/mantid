set(SRC_FILES
    src/MantidWebServiceAPIHelper.cpp
    src/MantidWebServiceAPIJobManager.cpp
    src/LSFJobManager.cpp
    src/SCARFLSFJobManager.cpp
    src/SimpleJSON.cpp)

set(INC_FILES
    inc/MantidRemoteJobManagers/MantidWebServiceAPIHelper.h
    inc/MantidRemoteJobManagers/MantidWebServiceAPIJobManager.h
    inc/MantidRemoteJobManagers/LSFJobManager.h
    inc/MantidRemoteJobManagers/SCARFLSFJobManager.h
    inc/MantidRemoteJobManagers/SimpleJSON.h)

set(TEST_FILES
    MantidWebServiceAPIHelperTest.h
    MantidWebServiceAPIJobManagerTest.h
    LSFJobManagerTest.h
    SCARFLSFJobManagerTest.h)

# set ( TEST_PY_FILES
#
# ) No Python tests yet...

if(COVERALLS)
  foreach(loop_var ${SRC_FILES} ${INC_FILES})
    set_property(GLOBAL APPEND
                 PROPERTY COVERAGE_SRCS
                          "${CMAKE_CURRENT_SOURCE_DIR}/${loop_var}")
  endforeach(loop_var)
endif()

# Not for now, remember later if convenient: Add a precompiled header where they
# are supported enable_precompiled_headers (
# inc/MantidRemoteJobManagers/PrecompiledHeader.h SRC_FILES )

# Add the target for this directory
add_library(RemoteJobManagers ${SRC_FILES} ${INC_FILES})

# Set the name of the generated library
set_target_properties(RemoteJobManagers
                      PROPERTIES OUTPUT_NAME MantidRemoteJobManagers)

if(OSX_VERSION VERSION_GREATER 10.8)
  set_target_properties(RemoteJobManagers
                        PROPERTIES INSTALL_RPATH
                                   "@loader_path/../Contents/MacOS")
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  set_target_properties(RemoteJobManagers
                        PROPERTIES INSTALL_RPATH "\$ORIGIN/../${LIB_DIR}")
endif()

# Add to the 'Framework' group in VS
set_property(TARGET RemoteJobManagers PROPERTY FOLDER "MantidFramework")

target_include_directories(RemoteJobManagers PUBLIC inc)
target_include_directories(RemoteJobManagers PRIVATE ${OPENSSL_INCLUDE_DIR})

target_link_libraries(RemoteJobManagers
                      LINK_PRIVATE
                      ${TCMALLOC_LIBRARIES_LINKTIME}
                      ${MANTIDLIBS}
                      ${GSL_LIBRARIES})

# Add the unit tests directory
add_subdirectory(test)

#
# Installation settings
#

mtd_install_targets(TARGETS
                    RemoteJobManagers
                    INSTALL_DIRS
                    ${PLUGINS_DIR}
                    ${WORKBENCH_PLUGINS_DIR})
