# Mantid source
set(SRC_FILES src/ScriptRepositoryImpl.cpp)

set(INC_FILES inc/MantidScriptRepository/ScriptRepositoryImpl.h)

set(TEST_FILES ScriptRepositoryTestImpl.h)

if(COVERAGE)
  foreach(loop_var ${SRCS_FILES} ${INC_FILES})
    set_property(GLOBAL APPEND PROPERTY COVERAGE_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/${loop_var}")
  endforeach(loop_var)
endif()

# Add the target for this directory
add_library(ScriptRepository ${SRC_FILES} ${INC_FILES})
add_library(Mantid::ScriptRepository ALIAS ScriptRepository)
target_include_directories(
  ScriptRepository PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc>
                          $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}> $<INSTALL_INTERFACE:include/Mantid>
)

# Auto-generate exports header
generate_mantid_export_header(
  ScriptRepository
  FALSE
  BASE_NAME
  MANTID_SCRIPTREPOSITORY
  EXPORT_MACRO_NAME
  SCRIPT_DLL_EXPORT
  EXPORT_FILE_NAME
  MantidScriptRepository/DllConfig.h
)

# Add the unit tests directory
add_subdirectory(test)
# Set the name of the generated library
set_target_properties(ScriptRepository PROPERTIES OUTPUT_NAME MantidScriptRepository)
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  set_target_properties(ScriptRepository PROPERTIES INSTALL_RPATH "@loader_path/../Contents/MacOS")
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  set_target_properties(ScriptRepository PROPERTIES INSTALL_RPATH "\$ORIGIN/../${LIB_DIR}")
endif()

set_property(TARGET ScriptRepository PROPERTY FOLDER "MantidFramework")

target_link_libraries(
  ScriptRepository
  PUBLIC Mantid::Kernel Mantid::API
  PRIVATE Mantid::Json
)

mtd_install_framework_lib(TARGETS ScriptRepository PLUGIN_LIB)
