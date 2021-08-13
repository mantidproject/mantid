set(SRC_FILES
    src/AbortRemoteJob2.cpp
    src/Authenticate2.cpp
    src/DownloadRemoteFile2.cpp
    src/Logout2.cpp
    src/QueryAllRemoteJobs2.cpp
    src/QueryRemoteFile2.cpp
    src/QueryRemoteJob2.cpp
    src/StartRemoteTransaction2.cpp
    src/StopRemoteTransaction2.cpp
    src/SubmitRemoteJob2.cpp
    src/UploadRemoteFile2.cpp)

set(INC_FILES
    inc/MantidRemoteAlgorithms/AbortRemoteJob2.h
    inc/MantidRemoteAlgorithms/Authenticate2.h
    inc/MantidRemoteAlgorithms/DownloadRemoteFile2.h
    inc/MantidRemoteAlgorithms/Logout2.h
    inc/MantidRemoteAlgorithms/QueryAllRemoteJobs2.h
    inc/MantidRemoteAlgorithms/QueryRemoteFile2.h
    inc/MantidRemoteAlgorithms/QueryRemoteJob2.h
    inc/MantidRemoteAlgorithms/StartRemoteTransaction2.h
    inc/MantidRemoteAlgorithms/StopRemoteTransaction2.h
    inc/MantidRemoteAlgorithms/SubmitRemoteJob2.h
    inc/MantidRemoteAlgorithms/UploadRemoteFile2.h)

set(TEST_FILES
    AbortRemoteJob2Test.h
    Authenticate2Test.h
    DownloadRemoteFile2Test.h
    Logout2Test.h
    QueryAllRemoteJobs2Test.h
    QueryRemoteFile2Test.h
    QueryRemoteJob2Test.h
    StartRemoteTransaction2Test.h
    StopRemoteTransaction2Test.h
    SubmitRemoteJob2Test.h
    UploadRemoteFile2Test.h)

if(COVERAGE)
  foreach(loop_var ${SRC_FILES} ${INC_FILES})
    set_property(GLOBAL APPEND
                 PROPERTY COVERAGE_SRCS
                          "${CMAKE_CURRENT_SOURCE_DIR}/${loop_var}")
  endforeach(loop_var)
endif()

# Not for now, remember later if convenient: Add a precompiled header where they
# are supported enable_precompiled_headers (
# inc/MantidRemoteAlgorithms/PrecompiledHeader.h SRC_FILES )

# Add the target for this directory
add_library(RemoteAlgorithms ${SRC_FILES} ${INC_FILES})
add_library(Mantid::RemoteAlgorithms ALIAS RemoteAlgorithms)

target_include_directories(RemoteAlgorithms PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc>
		$<INSTALL_INTERFACE:include/Mantid>
)

# Set the name of the generated library
set_target_properties(RemoteAlgorithms
                      PROPERTIES OUTPUT_NAME MantidRemoteAlgorithms)

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  set_target_properties(RemoteAlgorithms
                        PROPERTIES INSTALL_RPATH
                                   "@loader_path/../Contents/MacOS")
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  set_target_properties(RemoteAlgorithms
                        PROPERTIES INSTALL_RPATH "\$ORIGIN/../${LIB_DIR}")
endif()

# Add to the 'Framework' group in VS
set_property(TARGET RemoteAlgorithms PROPERTY FOLDER "MantidFramework")

target_link_libraries(RemoteAlgorithms PUBLIC Mantid::API PRIVATE Mantid::Kernel)

# Add the unit tests directory
add_subdirectory(test) # Note: No tests yet for many remote algorithms...

# Installation settings
if (CONDA_BUILD)
mtd_install_framework_lib(TARGETS RemoteAlgorithms PLUGIN_LIB)
else()
mtd_install_targets(TARGETS
                    RemoteAlgorithms
                    INSTALL_DIRS
                    ${PLUGINS_DIR}
                    ${WORKBENCH_PLUGINS_DIR})
endif()
