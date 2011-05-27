set ( SRC_FILES
	src/LoadIsawUB.cpp
	src/LoadPeaksFile.cpp
	src/MaskPeaksWorkspace.cpp
	src/PeakIntegration.cpp
	src/PredictPeaks.cpp
	src/SaveIsawPeaks.cpp
	src/SavePeaksFile.cpp
	src/SetGoniometer.cpp
)

set ( SRC_UNITY_IGNORE_FILES )
set ( INC_FILES
	inc/MantidCrystal/LoadIsawUB.h
	inc/MantidCrystal/LoadPeaksFile.h
	inc/MantidCrystal/MaskPeaksWorkspace.h
	inc/MantidCrystal/PeakIntegration.h
	inc/MantidCrystal/PredictPeaks.h
	inc/MantidCrystal/SaveIsawPeaks.h
	inc/MantidCrystal/SavePeaksFile.h
	inc/MantidCrystal/SetGoniometer.h
)

set ( TEST_FILES
	test/LoadIsawUBTest.h
	test/LoadPeaksFileTest.h
	test/MaskPeaksWorkspaceTest.h
	test/PredictPeaksTest.h
	test/SaveIsawPeaksTest.h
	test/SavePeaksFileTest.h
	test/SetGoniometerTest.h
)

if(UNITY_BUILD)
  include(UnityBuild)
  enable_unity_build(Crystal SRC_FILES SRC_UNITY_IGNORE_FILES 10)
endif(UNITY_BUILD)

# Add the target for this directory
add_library ( Crystal ${SRC_FILES} ${INC_FILES})
# Set the name of the generated library
set_target_properties ( Crystal PROPERTIES OUTPUT_NAME MantidCrystal )
# Add to the 'Framework' group in VS
set_property ( TARGET Crystal PROPERTY FOLDER "MantidFramework" )

include_directories ( inc ../DataObjects/inc)
include_directories ( ../MDEvents/inc)

target_link_libraries ( Crystal ${MANTIDLIBS} ${GSL_LIBRARIES} DataObjects)

if ( CXXTEST_FOUND )
  include_directories ( ../CurveFitting/inc ../DataHandling/inc ../Nexus/inc ../TestHelpers/inc ../Kernel/inc ../DataObjects/inc)
  cxxtest_add_test ( CrystalTest ${TEST_FILES} )
  target_link_libraries ( CrystalTest Kernel API DataHandling Crystal DataObjects TestHelpers)
  add_dependencies ( FrameworkTests CrystalTest )
  # Add to the 'FrameworkTests' group in VS
  set_property ( TARGET CrystalTest PROPERTY FOLDER "UnitTests" )
endif ()

###########################################################################
# Installation settings
###########################################################################

install ( TARGETS Crystal DESTINATION ${PLUGINS_DIR} )
