set(SRC_FILES
    src/Core/DateAndTime.cpp
    src/Core/DateAndTimeHelpers.cpp
    src/Event/TofEvent.cpp)

set(INC_FILES
    inc/MantidTypes/Core/DateAndTime.h
    inc/MantidTypes/Core/DateAndTimeHelpers.h
    inc/MantidTypes/Event/TofEvent.h
    inc/MantidTypes/SpectrumDefinition.h)

set(TEST_FILES
    DateAndTimeTest.h
    DateAndTimeHelpersTest.h
    SpectrumDefinitionTest.h
    TofEventTest.h)

if(COVERAGE)
  foreach(loop_var ${INC_FILES})
    set_property(GLOBAL APPEND
                 PROPERTY COVERAGE_SRCS
                          "${CMAKE_CURRENT_SOURCE_DIR}/${loop_var}")
  endforeach(loop_var)
endif()

add_library(Types ${SRC_FILES} ${INC_FILES})
add_library(Mantid::Types ALIAS Types)
set_target_properties(Types
                      PROPERTIES OUTPUT_NAME
                                 MantidTypes
                                 COMPILE_DEFINITIONS
                                 "Types_EXPORTS;PSAPI_VERSION=1")

set_property(TARGET Types PROPERTY FOLDER "MantidFramework")

target_include_directories(Types PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc>
		$<INSTALL_INTERFACE:include/Mantid>
)

# Auto-generate exports header
target_include_directories(Types PUBLIC
  $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/Framework/Types>
  $<INSTALL_INTERFACE:include/Mantid>
)

target_link_libraries(Types PUBLIC Boost::date_time Boost::regex)
generate_mantid_export_header(Types FALSE)

# Installation settings
if (CONDA_BUILD)
set(TARGET_EXPORT_NAME "MantidTypesTargets")
mtd_install_framework_lib(TARGETS Types INSTALL_EXPORT_FILE EXPORT_NAME ${TARGET_EXPORT_NAME})
else()
mtd_install_targets(TARGETS Types INSTALL_DIRS ${LIB_DIR} ${WORKBENCH_LIB_DIR})
endif()

# Add the unit tests directory
add_subdirectory(test)
