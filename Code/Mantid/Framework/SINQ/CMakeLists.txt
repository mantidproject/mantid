set ( SRC_FILES
	src/InvertMDDim.cpp
	src/LoadFlexiNexus.cpp
	src/MDHistoToWorkspace2D.cpp
	src/PoldiAutoCorrelation5.cpp
	src/PoldiAutoCorrelationCore.cpp
	src/PoldiBasicChopper.cpp
	src/PoldiChopperFactory.cpp
	src/PoldiDeadWireDecorator.cpp
	src/PoldiDetectorDecorator.cpp
	src/PoldiDetectorFactory.cpp
	src/PoldiHeliumDetector.cpp
	src/PoldiLoadChopperSlits.cpp
	src/PoldiLoadIPP.cpp
	src/PoldiLoadLog.cpp
	src/PoldiLoadSpectra.cpp
	src/PoldiPeakDetection2.cpp
	src/PoldiRemoveDeadWires.cpp
	src/ProjectMD.cpp
	src/SINQHMListener.cpp
	src/SINQTranspose3D.cpp
	src/SliceMDHisto.cpp
)

set ( INC_FILES
	inc/MantidSINQ/DllConfig.h
	inc/MantidSINQ/InvertMDDim.h
	inc/MantidSINQ/LoadFlexiNexus.h
	inc/MantidSINQ/MDHistoToWorkspace2D.h
	inc/MantidSINQ/PoldiAbstractChopper.h
	inc/MantidSINQ/PoldiAbstractDetector.h
	inc/MantidSINQ/PoldiAutoCorrelation5.h
	inc/MantidSINQ/PoldiAutoCorrelationCore.h
	inc/MantidSINQ/PoldiBasicChopper.h
	inc/MantidSINQ/PoldiChopperFactory.h
	inc/MantidSINQ/PoldiDeadWireDecorator.h
	inc/MantidSINQ/PoldiDetectorDecorator.h
	inc/MantidSINQ/PoldiDetectorFactory.h
	inc/MantidSINQ/PoldiHeliumDetector.h
	inc/MantidSINQ/PoldiLoadChopperSlits.h
	inc/MantidSINQ/PoldiLoadIPP.h
	inc/MantidSINQ/PoldiLoadLog.h
	inc/MantidSINQ/PoldiLoadSpectra.h
	inc/MantidSINQ/PoldiMockInstrumentHelpers.h
	inc/MantidSINQ/PoldiPeakDetection2.h
	inc/MantidSINQ/PoldiRemoveDeadWires.h
	inc/MantidSINQ/ProjectMD.h
	inc/MantidSINQ/SINQHMListener.h
	inc/MantidSINQ/SINQTranspose3D.h
	inc/MantidSINQ/SliceMDHisto.h
)

set ( TEST_FILES
	InvertMDDimTest.h
	LoadFlexiNexusTest.h
	MDHistoToWorkspace2DTest.h
	PoldiAutoCorrelationCoreTest.h
	PoldiBasicChopperTest.h
	PoldiChopperFactoryTest.h
	PoldiDeadWireDecoratorTest.h
	PoldiDetectorDecoratorTest.h
	PoldiDetectorFactoryTest.h
	PoldiDetectorTest.h
	ProjectMDTest.h
	SliceMDHistoTest.h
)


# Add a precompiled header where they are supported
enable_precompiled_headers ( inc/MantidSINQ/PrecompiledHeader.h SRC_FILES )
# Add the target for this directory
add_library ( SINQ ${SRC_FILES} ${INC_FILES} )
# Set the name of the generated library
set_target_properties ( SINQ PROPERTIES OUTPUT_NAME MantidSINQ 
                                        COMPILE_DEFINITIONS "IN_MANTID_SINQ" )
# Add to the 'Framework' group in VS
set_property ( TARGET SINQ PROPERTY FOLDER "MantidFramework" )

include_directories ( inc ../MDEvents/inc )

target_link_libraries ( SINQ ${MANTIDLIBS} MDEvents )

# Add the unit tests directory
add_subdirectory ( test )

###########################################################################
# Installation settings
###########################################################################

install ( TARGETS SINQ ${SYSTEM_PACKAGE_TARGET} DESTINATION ${PLUGINS_DIR} )
