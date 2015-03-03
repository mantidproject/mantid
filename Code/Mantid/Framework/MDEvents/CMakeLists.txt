# Source files
set ( SRC_FILES
	#
	src/BoxControllerSettingsAlgorithm.cpp
	src/CalculateReflectometryQBase.cpp
	src/ConvToMDBase.cpp
	src/ConvToMDEventsWS.cpp
	src/ConvToMDHistoWS.cpp
	src/ConvToMDSelector.cpp
	src/ConvertToReflectometryQ.cpp
	src/FitMD.cpp
	src/ImportMDEventWorkspace.cpp
	src/ImportMDHistoWorkspace.cpp
	src/ImportMDHistoWorkspaceBase.cpp
	src/Integrate3DEvents.cpp
	src/IntegrateEllipsoids.cpp
        src/MDEventWSWrapper.cpp
	src/MDTransfAxisNames.cpp
	src/MDTransfFactory.cpp
	src/MDTransfModQ.cpp
	src/MDTransfNoQ.cpp
	src/MDTransfQ3D.cpp
	src/MDWSDescription.cpp
	src/MDWSTransform.cpp
	src/OneStepMDEW.cpp
	src/QueryMDWorkspace.cpp
	src/ReflectometryTransform.cpp
	src/ReflectometryTransformKiKf.cpp
	src/ReflectometryTransformP.cpp
	src/ReflectometryTransformQxQz.cpp
	src/SaveIsawQvector.cpp
	src/UnitsConversionHelper.cpp
	src/UserFunctionMD.cpp
)

set ( SRC_UNITY_IGNORE_FILES 
    src/MDBoxBase.cpp
	src/MDBin.cpp
    src/MDBox.cpp
    src/MDBoxIterator.cpp
    src/MDEventWorkspace.cpp
    src/MDGridBox.cpp
)

# Header files
set ( INC_FILES
	#
	inc/MantidMDEvents/BoxControllerSettingsAlgorithm.h
	inc/MantidMDEvents/CalculateReflectometryQBase.h
	inc/MantidMDEvents/ConvToMDBase.h
	inc/MantidMDEvents/ConvToMDEventsWS.h
	inc/MantidMDEvents/ConvToMDHistoWS.h
	inc/MantidMDEvents/ConvToMDSelector.h
	inc/MantidMDEvents/ConvertToReflectometryQ.h
	inc/MantidMDEvents/FitMD.h
	inc/MantidMDEvents/ImportMDEventWorkspace.h
	inc/MantidMDEvents/ImportMDHistoWorkspace.h
	inc/MantidMDEvents/ImportMDHistoWorkspaceBase.h
	inc/MantidMDEvents/Integrate3DEvents.h
	inc/MantidMDEvents/IntegrateEllipsoids.h
        inc/MantidMDEvents/MDEventWSWrapper.h
	inc/MantidMDEvents/MDTransfAxisNames.h
	inc/MantidMDEvents/MDTransfFactory.h
	inc/MantidMDEvents/MDTransfInterface.h
	inc/MantidMDEvents/MDTransfModQ.h
	inc/MantidMDEvents/MDTransfNoQ.h
	inc/MantidMDEvents/MDTransfQ3D.h
	inc/MantidMDEvents/MDWSDescription.h
	inc/MantidMDEvents/MDWSTransform.h
	inc/MantidMDEvents/OneStepMDEW.h
	inc/MantidMDEvents/QueryMDWorkspace.h
	inc/MantidMDEvents/ReflectometryTransform.h
	inc/MantidMDEvents/ReflectometryTransformKiKf.h
	inc/MantidMDEvents/ReflectometryTransformP.h
	inc/MantidMDEvents/ReflectometryTransformQxQz.h
	inc/MantidMDEvents/SaveIsawQvector.h
	inc/MantidMDEvents/UnitsConversionHelper.h
	inc/MantidMDEvents/UserFunctionMD.h
)

# Test files.
set ( TEST_FILES
	BoxControllerSettingsAlgorithmTest.h
	ConvertToReflectometryQTest.h
	FitMDTest.h
	ImportMDEventWorkspaceTest.h
	ImportMDHistoWorkspaceTest.h
	Integrate3DEventsTest.h
        MDEventWSWrapperTest.h
	MDTransfAxisNamesTest.h
	MDTransfFactoryTest.h
	MDTransfModQTest.h
	MDTransfQ3DTest.h
	MDWSDescriptionTest.h
	MDWSTransfTest.h
	OneStepMDEWTest.h
	QueryMDWorkspaceTest.h
	ReflectometryTransformKiKfTest.h
	ReflectometryTransformPTest.h
	ReflectometryTransformQxQzTest.h
	SaveIsawQvectorTest.h
	SkippingPolicyTest.h
	UnitsConversionHelperTest.h
)


if(UNITY_BUILD)
  include(UnityBuild)
  enable_unity_build(MDEvents SRC_FILES SRC_UNITY_IGNORE_FILES 10)
endif(UNITY_BUILD)

# Use a precompiled header where they are supported
enable_precompiled_headers( inc/MantidMDEvents/PrecompiledHeader.h SRC_FILES )
# Add the target for this directory
add_library ( MDEvents ${SRC_FILES} ${INC_FILES})
# Set the name of the generated library
set_target_properties ( MDEvents PROPERTIES OUTPUT_NAME MantidMDEvents 
                                                 COMPILE_DEFINITIONS IN_MANTID_MDEVENT )

if (OSX_VERSION VERSION_GREATER 10.8)
  set_target_properties ( MDEvents PROPERTIES INSTALL_RPATH "@loader_path/../MacOS")
endif () 

# Intensive use of templated libaries can cause large objects to be generated. These require
# an additional flag in MSVC.
if ( MSVC )
  set_target_properties ( MDEvents PROPERTIES COMPILE_FLAGS "/bigobj" )
endif ()
                                                 
# Add to the 'Framework' group in VS
set_property ( TARGET MDEvents PROPERTY FOLDER "MantidFramework" )

include_directories( inc )
target_link_libraries ( MDEvents ${MANTIDLIBS} )

# Add the unit tests directory
add_subdirectory ( test )

###########################################################################
# Installation settings
###########################################################################

install ( TARGETS MDEvents ${SYSTEM_PACKAGE_TARGET} DESTINATION ${LIB_DIR} )
