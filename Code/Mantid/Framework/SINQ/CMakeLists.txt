set ( SRC_FILES
	src/InvertMDDim.cpp
	src/LoadFlexiNexus.cpp
	src/MDHistoToWorkspace2D.cpp
	src/PoldiAnalyseResiduals.cpp
	src/PoldiAutoCorrelation5.cpp
	src/PoldiCreatePeaksFromCell.cpp
	src/PoldiFitPeaks1D.cpp
        src/PoldiFitPeaks1D2.cpp
        src/PoldiFitPeaks2D.cpp
	src/PoldiIndexKnownCompounds.cpp
	src/PoldiLoadChopperSlits.cpp
	src/PoldiLoadIPP.cpp
	src/PoldiLoadLog.cpp
	src/PoldiLoadSpectra.cpp
	src/PoldiPeakDetection2.cpp
	src/PoldiPeakSearch.cpp
	src/PoldiPeakSummary.cpp
	src/PoldiRemoveDeadWires.cpp
	src/PoldiTruncateData.cpp
	src/PoldiUtilities/IPoldiFunction1D.cpp
	src/PoldiUtilities/MillerIndices.cpp
        src/PoldiUtilities/PeakFunctionIntegrator.cpp
	src/PoldiUtilities/Poldi2DFunction.cpp
	src/PoldiUtilities/PoldiAutoCorrelationCore.cpp
	src/PoldiUtilities/PoldiBasicChopper.cpp
	src/PoldiUtilities/PoldiChopperFactory.cpp
        src/PoldiUtilities/PoldiConversions.cpp
	src/PoldiUtilities/PoldiDeadWireDecorator.cpp
	src/PoldiUtilities/PoldiDetectorDecorator.cpp
	src/PoldiUtilities/PoldiDetectorFactory.cpp
        src/PoldiUtilities/PoldiDGrid.cpp
	src/PoldiUtilities/PoldiHeliumDetector.cpp
        src/PoldiUtilities/PoldiInstrumentAdapter.cpp
	src/PoldiUtilities/PoldiPeak.cpp
	src/PoldiUtilities/PoldiPeakCollection.cpp
	src/PoldiUtilities/PoldiResidualCorrelationCore.cpp
        src/PoldiUtilities/PoldiSpectrumDomainFunction.cpp
        src/PoldiUtilities/PoldiSourceSpectrum.cpp
	src/PoldiUtilities/PoldiSpectrumConstantBackground.cpp
	src/PoldiUtilities/PoldiSpectrumLinearBackground.cpp
	src/PoldiUtilities/PoldiTimeTransformer.cpp
	src/PoldiUtilities/UncertainValue.cpp
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
	inc/MantidSINQ/PoldiAnalyseResiduals.h
	inc/MantidSINQ/PoldiAutoCorrelation5.h
	inc/MantidSINQ/PoldiCreatePeaksFromCell.h
	inc/MantidSINQ/PoldiFitPeaks1D.h
        inc/MantidSINQ/PoldiFitPeaks1D2.h
        inc/MantidSINQ/PoldiFitPeaks2D.h
	inc/MantidSINQ/PoldiIndexKnownCompounds.h
	inc/MantidSINQ/PoldiLoadChopperSlits.h
	inc/MantidSINQ/PoldiLoadIPP.h
	inc/MantidSINQ/PoldiLoadLog.h
	inc/MantidSINQ/PoldiLoadSpectra.h
	inc/MantidSINQ/PoldiPeakDetection2.h
	inc/MantidSINQ/PoldiPeakSearch.h
	inc/MantidSINQ/PoldiPeakSummary.h
	inc/MantidSINQ/PoldiRemoveDeadWires.h
	inc/MantidSINQ/PoldiTruncateData.h
	inc/MantidSINQ/PoldiUtilities/IPoldiFunction1D.h
	inc/MantidSINQ/PoldiUtilities/MillerIndices.h
	inc/MantidSINQ/PoldiUtilities/MillerIndicesIO.h
        inc/MantidSINQ/PoldiUtilities/PeakFunctionIntegrator.h
	inc/MantidSINQ/PoldiUtilities/Poldi2DFunction.h
	inc/MantidSINQ/PoldiUtilities/PoldiAbstractChopper.h
	inc/MantidSINQ/PoldiUtilities/PoldiAbstractDetector.h
	inc/MantidSINQ/PoldiUtilities/PoldiAutoCorrelationCore.h
	inc/MantidSINQ/PoldiUtilities/PoldiBasicChopper.h
	inc/MantidSINQ/PoldiUtilities/PoldiChopperFactory.h
        inc/MantidSINQ/PoldiUtilities/PoldiConversions.h
	inc/MantidSINQ/PoldiUtilities/PoldiDeadWireDecorator.h
	inc/MantidSINQ/PoldiUtilities/PoldiDetectorDecorator.h
	inc/MantidSINQ/PoldiUtilities/PoldiDetectorFactory.h
        inc/MantidSINQ/PoldiUtilities/PoldiDGrid.h
	inc/MantidSINQ/PoldiUtilities/PoldiHeliumDetector.h
        inc/MantidSINQ/PoldiUtilities/PoldiInstrumentAdapter.h
	inc/MantidSINQ/PoldiUtilities/PoldiMockInstrumentHelpers.h
	inc/MantidSINQ/PoldiUtilities/PoldiPeak.h
	inc/MantidSINQ/PoldiUtilities/PoldiPeakCollection.h
	inc/MantidSINQ/PoldiUtilities/PoldiResidualCorrelationCore.h
	inc/MantidSINQ/PoldiUtilities/PoldiSpectrumConstantBackground.h
        inc/MantidSINQ/PoldiUtilities/PoldiSpectrumDomainFunction.h
	inc/MantidSINQ/PoldiUtilities/PoldiSpectrumLinearBackground.h
	inc/MantidSINQ/PoldiUtilities/PoldiTimeTransformer.h
	inc/MantidSINQ/PoldiUtilities/UncertainValue.h
	inc/MantidSINQ/PoldiUtilities/UncertainValueIO.h
	inc/MantidSINQ/ProjectMD.h
	inc/MantidSINQ/SINQHMListener.h
	inc/MantidSINQ/SINQTranspose3D.h
	inc/MantidSINQ/SliceMDHisto.h
)

set ( TEST_FILES
	InvertMDDimTest.h
	LoadFlexiNexusTest.h
	MDHistoToWorkspace2DTest.h
	MillerIndicesIOTest.h
	MillerIndicesTest.h
        PeakFunctionIntegratorTest.h
	Poldi2DFunctionTest.h
	PoldiAnalyseResidualsTest.h
	PoldiAutoCorrelationCoreTest.h
	PoldiBasicChopperTest.h
	PoldiChopperFactoryTest.h
        PoldiConversionsTest.h
	PoldiCreatePeaksFromCellTest.h
	PoldiDeadWireDecoratorTest.h
	PoldiDetectorDecoratorTest.h
        PoldiDGridTest.h
	PoldiDetectorFactoryTest.h
	PoldiDetectorTest.h
	PoldiFitPeaks1DTest.h
        PoldiFitPeaks1D2Test.h
        PoldiFitPeaks2DTest.h
	PoldiIndexKnownCompoundsTest.h
        PoldiInstrumentAdapterTest.h
        PoldiPeakCollectionTest.h
	PoldiPeakSearchTest.h
	PoldiPeakSummaryTest.h
	PoldiPeakTest.h
	PoldiResidualCorrelationCoreTest.h
        PoldiSourceSpectrumTest.h
        PoldiSpectrumDomainFunctionTest.h
	PoldiSpectrumConstantBackgroundTest.h
	PoldiSpectrumLinearBackgroundTest.h
	PoldiTimeTransformerTest.h
	PoldiTruncateDataTest.h
	ProjectMDTest.h
	SliceMDHistoTest.h
	UncertainValueIOTest.h
	UncertainValueTest.h
)


# Add a precompiled header where they are supported
enable_precompiled_headers ( inc/MantidSINQ/PrecompiledHeader.h SRC_FILES )
# Add the target for this directory
add_library ( SINQ ${SRC_FILES} ${INC_FILES} )
# Set the name of the generated library
set_target_properties ( SINQ PROPERTIES OUTPUT_NAME MantidSINQ 
                                        COMPILE_DEFINITIONS "IN_MANTID_SINQ" 
                                        INSTALL_RPATH "@loader_path/../Contents/MacOS/")
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
