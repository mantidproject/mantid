set ( SRC_FILES
	src/ConvertFitFunctionForMuonTFAsymmetry.cpp
)

set ( INC_FILES
	inc/MantidMuon/ConvertFitFunctionForMuonTFAsymmetry.h
)

set ( TEST_FILES 
	ApplyMuonDetectorsGroupingTest.h
	MuonAlgorithmHelperTest.h
  ConvertFitFunctionForMuonTFAsymmetryTest.h)

if (COVERALLS)
    foreach( loop_var ${SRC_FILES} ${C_SRC_FILES} ${INC_FILES})
      set_property(GLOBAL APPEND PROPERTY COVERAGE_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/${loop_var}")
    endforeach(loop_var)
endif()

# Add a precompiled header where they are supported
# enable_precompiled_headers ( inc/MantidAlgorithms/PrecompiledHeader.h SRC_FILES )
# Add the target for this directory
add_library ( Muon ${SRC_FILES} ${C_SRC_FILES} ${INC_FILES})
# Set the name of the generated library
set_target_properties ( Muon PROPERTIES OUTPUT_NAME MantidMuon
  COMPILE_DEFINITIONS "IN_MANTID_MUON"
)

if (OSX_VERSION VERSION_GREATER 10.8)
  set_target_properties(Muon PROPERTIES INSTALL_RPATH "@loader_path/../Contents/MacOS")
elseif ( ${CMAKE_SYSTEM_NAME} STREQUAL "Linux" )
  set_target_properties(Muon PROPERTIES INSTALL_RPATH "\$ORIGIN/../${LIB_DIR}")
endif ()

# Add to the 'Framework' group in VS
set_property ( TARGET Muon PROPERTY FOLDER "MantidFramework" )

include_directories ( inc )

target_link_libraries ( Muon LINK_PRIVATE ${TCMALLOC_LIBRARIES_LINKTIME} ${MANTIDLIBS} ${GSL_LIBRARIES} )

# Add the unit tests directory
add_subdirectory ( test )

###########################################################################
# Installation settings
###########################################################################

install ( TARGETS Muon ${SYSTEM_PACKAGE_TARGET} DESTINATION ${PLUGINS_DIR} )
