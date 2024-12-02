include(GenerateExportHeader)

set(SRC_FILES
    src/napi.c
    src/napi4.c
    src/napi5.c
    src/NeXusException.cpp
    src/NeXusFile.cpp
    src/nxstack.c
    src/stptok.c
)

set(INC_FILES
    inc/MantidNexusCpp/napi.h
    inc/MantidNexusCpp/napi4.h
    inc/MantidNexusCpp/napi5.h
    inc/MantidNexusCpp/napiconfig.h
    inc/MantidNexusCpp/napi_internal.h
    inc/MantidNexusCpp/NeXusException.hpp
    inc/MantidNexusCpp/NeXusFile.hpp
    inc/MantidNexusCpp/nx_stptok.h
    inc/MantidNexusCpp/nxconfig.h
    inc/MantidNexusCpp/nxstack.h
)

# Add the target for this directory
add_library(NexusCpp ${SRC_FILES} ${INC_FILES})
add_library(Mantid::NexusCpp ALIAS NexusCpp)

target_include_directories(
  NexusCpp PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc> $<INSTALL_INTERFACE:include/Mantid>
)

# Set the name of the generated library
set_target_properties(
  NexusCpp
  PROPERTIES OUTPUT_NAME MantidNexusCpp
             COMPILE_DEFINITIONS IN_MANTID_NEXUSCPP
             WITH_HDF4 WITH_HDF5
)
target_link_libraries(NexusCpp PUBLIC ${HDF4_DF_LIBRARY} ${HDF4_MFHDF_LIBRARY} ${HDF5_LIBRARIES} ${HDF5_HL_LIBRARIES})

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  set_target_properties(NexusCpp PROPERTIES INSTALL_RPATH "@loader_path/../MacOS;@loader_path/../Frameworks")
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  set_target_properties(NexusCpp PROPERTIES INSTALL_RPATH "\$ORIGIN/../${LIB_DIR}")
endif()

# Add to the 'Framework' group in VS
set_property(TARGET NexusCpp PROPERTY FOLDER "MantidFramework")

# Auto-generate exports header
target_include_directories(
  NexusCpp PUBLIC $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/Framework/NexusCpp> $<INSTALL_INTERFACE:include/Mantid>
)
generate_mantid_export_header(NexusCpp FALSE)

# ######### TESTS
set(TEST_PREFIX "NexusCppTest")

# #### HDF5
add_executable(napi_test_hdf5 EXCLUDE_FROM_ALL test/napi_test.c)
target_link_libraries(napi_test_hdf5 NexusCpp)
add_test(NAME "${TEST_PREFIX}-C-HDF5-test" COMMAND napi_test_hdf5)

add_executable(napi_attra_test_hdf5 EXCLUDE_FROM_ALL test/napi_attra_test.c)
target_link_libraries(napi_attra_test_hdf5 NexusCpp)
add_test(NAME "${TEST_PREFIX}-C-HDF5-attra-test" COMMAND napi_attra_test_hdf5)

# TODO figure out correct check

# if (WIN32) set_property(TEST "${TEST_PREFIX}-C-HDF5-test" "${TEST_PREFIX}-C-HDF5-attra-test" APPEND PROPERTY
# ENVIRONMENT "PATH=${TESTSPATH}") endif(WIN32)

add_executable(napi_test_cpp-hdf5 EXCLUDE_FROM_ALL test/napi_test_cpp.cpp)
target_link_libraries(napi_test_cpp-hdf5 NexusCpp)
add_test(NAME "${TEST_PREFIX}-Cpp-HDF5-test" COMMAND napi_test_cpp-hdf5)

# TODO figure out correct check

# if (WIN32) set_property(TEST "${TEST_PREFIX}-Cpp-HDF5-test" APPEND PROPERTY ENVIRONMENT "PATH=${TESTSPATH}")
# endif(WIN32)

# #### HDF4
add_executable(napi_test_hdf4 EXCLUDE_FROM_ALL test/napi_test.c)
target_link_libraries(napi_test_hdf4 NexusCpp)
add_test(NAME "${TEST_PREFIX}-C-HDF4-test" COMMAND napi_test_hdf4)

add_executable(napi_attra_test_hdf4 EXCLUDE_FROM_ALL test/napi_attra_test.c)
target_link_libraries(napi_attra_test_hdf4 NexusCpp)
add_test(NAME "${TEST_PREFIX}-C-HDF4-attra-test" COMMAND napi_attra_test_hdf4)
set_tests_properties("${TEST_PREFIX}-C-HDF4-attra-test" PROPERTIES WILL_FAIL TRUE)

# TODO figure out correct check

# if(WIN32) set_property( TEST "${TEST_PREFIX}-C-HDF4-test" "${TEST_PREFIX}-C-HDF4-attra-test" APPEND PROPERTY
# ENVIRONMENT "PATH=${TESTSPATH}" ) endif(WIN32)

add_executable(napi_test_cpp-hdf4 EXCLUDE_FROM_ALL test/napi_test_cpp.cpp)
target_link_libraries(napi_test_cpp-hdf4 NexusCpp)
add_test(NAME "${TEST_PREFIX}-Cpp-HDF4-test" COMMAND napi_test_cpp-hdf4)

# TODO figure out correct check

# if (WIN32) set_property(TEST "${TEST_PREFIX}-Cpp-HDF4-test" APPEND PROPERTY ENVIRONMENT "PATH=${TESTSPATH}")
# endif(WIN32)

# #### test for unlimited dimensions
add_executable(napi_test_nxunlimited EXCLUDE_FROM_ALL test/test_nxunlimited.c)
target_link_libraries(napi_test_nxunlimited NexusCpp)
add_test(NAME "${TEST_PREFIX}-C-test-nxunlimited" COMMAND napi_test_nxunlimited)

# TODO figure out correct check

# if (WIN32) set_property(TEST "NAPI-C-test-nxunlimited" APPEND PROPERTY ENVIRONMENT "PATH=${TESTSPATH}") endif(WIN32)

# #### leak tests

add_executable(napi_leak_test1 EXCLUDE_FROM_ALL test/leak_test1.c)
target_link_libraries(napi_leak_test1 NexusCpp)
add_test(NAME "${TEST_PREFIX}-C-leak-test-1" COMMAND napi_leak_test1)

# TODO figure out correct check

# if (WIN32) set_property(TEST "${TEST_PREFIX}-C-leak-test-1" APPEND PROPERTY ENVIRONMENT "PATH=${TESTSPATH}")
# endif(WIN32)

add_executable(napi_leak_test2 EXCLUDE_FROM_ALL test/leak_test2.cpp)
target_link_libraries(napi_leak_test2 NexusCpp)
add_test(NAME "${TEST_PREFIX}-Cpp-leak-test-2" COMMAND napi_leak_test2)

# TODO figure out correct check

# if (WIN32) set_property(TEST "NAPI-C++-leak-test-2" APPEND PROPERTY ENVIRONMENT "PATH=${TESTSPATH}") endif(WIN32)

add_executable(napi_leak_test3 EXCLUDE_FROM_ALL test/leak_test3.cpp)
target_link_libraries(napi_leak_test3 NexusCpp)
add_test(NAME "${TEST_PREFIX}-Cpp-leak-test-3" COMMAND napi_leak_test3)

# TODO figure out correct check

# if (WIN32) set_property(TEST "NAPI-C++-leak-test-3" APPEND PROPERTY ENVIRONMENT "PATH=${TESTSPATH}") endif(WIN32)

# #### phony target to drag in all tests
add_custom_target(
  "${TEST_PREFIX}"
  DEPENDS napi_test_hdf5
          napi_attra_test_hdf5
          napi_test_cpp-hdf5
          napi_test_hdf4
          napi_attra_test_hdf4
          napi_test_cpp-hdf4
          napi_test_nxunlimited
          napi_leak_test1
          napi_leak_test2
          napi_leak_test3
)

add_dependencies(FrameworkTests "${TEST_PREFIX}")

# ######### Installation settings
set(TARGET_EXPORT_NAME "MantidNexusCppTargets")
mtd_install_framework_lib(TARGETS NexusCpp INSTALL_EXPORT_FILE EXPORT_NAME ${TARGET_EXPORT_NAME})
