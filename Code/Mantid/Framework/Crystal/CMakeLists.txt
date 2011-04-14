set ( SRC_FILES
	src/LoadPeaksFile.cpp
	src/PeakIntegration.cpp
	src/SavePeaksFile.cpp
)

set ( SRC_UNITY_IGNORE_FILES )
set ( INC_FILES
	inc/MantidCrystal/LoadPeaksFile.h
	inc/MantidCrystal/PeakIntegration.h
	inc/MantidCrystal/SavePeaksFile.h
)

set ( TEST_FILES
	test/LoadPeaksFileTest.h
	test/SavePeaksFileTest.h
)

if(UNITY_BUILD)
  include(UnityBuild)
  enable_unity_build(Crystal SRC_FILES SRC_UNITY_IGNORE_FILES 1000)
endif(UNITY_BUILD)

# Add the target for this directory
add_library ( Crystal ${SRC_FILES} ${INC_FILES})
# Set the name of the generated library
set_target_properties ( Crystal PROPERTIES OUTPUT_NAME MantidCrystal )
# Add to the 'Framework' group in VS
set_property ( TARGET Crystal PROPERTY FOLDER "MantidFramework" )

include_directories ( inc ../DataObjects/inc)

target_link_libraries ( Crystal ${MANTIDLIBS} ${GSL_LIBRARIES} DataObjects)

if ( CXXTEST_FOUND )
  include_directories ( ../CurveFitting/inc ../DataHandling/inc ../Nexus/inc ../TestHelpers/inc ../Kernel/inc ../DataObjects/inc)
  cxxtest_add_test ( CrystalTest ${TEST_FILES} )
  target_link_libraries ( CrystalTest Kernel API DataHandling Crystal DataObjects)
  add_dependencies ( FrameworkTests CrystalTest )
  # Add to the 'FrameworkTests' group in VS
  set_property ( TARGET CrystalTest PROPERTY FOLDER "UnitTests" )
endif ()

###########################################################################
# Installation settings
###########################################################################

install ( TARGETS Crystal DESTINATION ${PLUGINS_DIR} )
