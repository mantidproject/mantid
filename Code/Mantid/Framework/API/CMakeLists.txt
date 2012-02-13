set ( SRC_FILES
	src/Algorithm.cpp
	src/AlgorithmFactory.cpp
	src/AlgorithmHasProperty.cpp
	src/AlgorithmHistory.cpp
	src/AlgorithmManager.cpp
	src/AlgorithmProperty.cpp
	src/AlgorithmProxy.cpp
	src/AnalysisDataService.cpp
	src/ArchiveSearchFactory.cpp
	src/Axis.cpp
	src/BoxController.cpp
	src/CatalogFactory.cpp
	src/CloneableAlgorithm.cpp
	src/Column.cpp
	src/ColumnFactory.cpp
	src/CompositeDomainMD.cpp
	src/CompositeFunction.cpp
	src/ConstraintFactory.cpp
	src/CoordTransform.cpp
	src/CostFunctionFactory.cpp
	src/DeprecatedAlgorithm.cpp
	src/EnabledWhenWorkspaceIsType.cpp
	src/ExperimentInfo.cpp
	src/Expression.cpp
	src/FileFinder.cpp
	src/FileProperty.cpp
	src/FrameworkManager.cpp
	src/FunctionDomain1D.cpp
	src/FunctionDomainMD.cpp
	src/FunctionFactory.cpp
	src/FunctionProperty.cpp
	src/FunctionValues.cpp
	src/IDataFileChecker.cpp
	src/IEventList.cpp
	src/IEventWorkspace.cpp
	src/IFunction.cpp
	src/IFunctionMD.cpp
	src/IFunction1D.cpp
	src/IFunctionMW.cpp
	src/ILiveListener.cpp
	src/IMDEventWorkspace.cpp
	src/IMDHistoWorkspace.cpp
	src/IMDIterator.cpp
	src/IMDWorkspace.cpp
	src/IPeak.cpp
	src/IPeakFunction.cpp
	src/IPeaksWorkspace.cpp
	src/ISpectrum.cpp
	src/ITableWorkspace.cpp
	src/ImplicitFunctionParameterParserFactory.cpp
	src/ImplicitFunctionParserFactory.cpp
	src/ImplictFunctionFactory.cpp
	src/InstrumentDataService.cpp
	src/LiveListenerFactory.cpp
	src/LoadAlgorithmFactory.cpp
	src/LocatedDataRef.cpp
	src/LocatedDataValue.cpp
	src/MDGeometry.cpp
	src/MatrixWSIndexCalculator.cpp
	src/MatrixWorkspace.cpp
	src/MatrixWorkspaceMDIterator.cpp
	src/MemoryManager.cpp
	src/MultipleExperimentInfos.cpp
	src/MultipleFileProperty.cpp
	src/NullCoordTransform.cpp
	src/NumericAxis.cpp
	src/ParamFunction.cpp
	src/ParameterReference.cpp
	src/ParameterTie.cpp
	src/Progress.cpp
	src/PropertyNexus.cpp
	src/RefAxis.cpp
	src/Run.cpp
	src/Sample.cpp
	src/SampleEnvironment.cpp
	src/SpectraAxis.cpp
	src/SpectraDetectorMap.cpp
	src/TableRow.cpp
	src/TextAxis.cpp
	src/Workspace.cpp
	src/WorkspaceFactory.cpp
	src/WorkspaceGroup.cpp
	src/WorkspaceHistory.cpp
	src/WorkspaceOpOverloads.cpp
	src/WorkspaceProperty.cpp
)

set ( SRC_UNITY_IGNORE_FILES src/CompositeFunction.cpp
        src/CompositeFunctionMW.cpp
        src/IDataFileChecker.cpp
        src/MatrixWorkspace.cpp
        src/IEventWorkspace.cpp
        src/IFunctionMD.cpp
        src/IPeakFunction.cpp
        )

set ( INC_FILES
	inc/MantidAPI/Algorithm.h
	inc/MantidAPI/AlgorithmFactory.h
	inc/MantidAPI/AlgorithmHasProperty.h
	inc/MantidAPI/AlgorithmHistory.h
	inc/MantidAPI/AlgorithmManager.h
	inc/MantidAPI/AlgorithmObserver.h
	inc/MantidAPI/AlgorithmProperty.h
	inc/MantidAPI/AlgorithmProxy.h
	inc/MantidAPI/AnalysisDataService.h
	inc/MantidAPI/ArchiveSearchFactory.h
	inc/MantidAPI/Axis.h
	inc/MantidAPI/BoxController.h
	inc/MantidAPI/CatalogFactory.h
	inc/MantidAPI/CloneableAlgorithm.h
	inc/MantidAPI/Column.h
	inc/MantidAPI/ColumnFactory.h
	inc/MantidAPI/CompositeFunction.h
	inc/MantidAPI/CompositeDomain.h
	inc/MantidAPI/CompositeDomainMD.h
	inc/MantidAPI/ConstraintFactory.h
	inc/MantidAPI/CoordTransform.h
	inc/MantidAPI/CostFunctionFactory.h
	inc/MantidAPI/DeclareUserAlg.h
	inc/MantidAPI/DeprecatedAlgorithm.h
	inc/MantidAPI/DllConfig.h
	inc/MantidAPI/EnabledWhenWorkspaceIsType.h
	inc/MantidAPI/ExperimentInfo.h
	inc/MantidAPI/Expression.h
	inc/MantidAPI/FileFinder.h
	inc/MantidAPI/FileProperty.h
	inc/MantidAPI/FrameworkManager.h
	inc/MantidAPI/FunctionDomain.h
	inc/MantidAPI/FunctionDomain1D.h
	inc/MantidAPI/FunctionDomainMD.h
	inc/MantidAPI/FunctionFactory.h
	inc/MantidAPI/FunctionProperty.h
	inc/MantidAPI/FunctionValues.h
	inc/MantidAPI/IAlgorithm.h
	inc/MantidAPI/IArchiveSearch.h
	inc/MantidAPI/IBackgroundFunction.h
	inc/MantidAPI/ICatalog.h
	inc/MantidAPI/IConstraint.h
	inc/MantidAPI/ICostFunction.h
	inc/MantidAPI/IDataFileChecker.h
	inc/MantidAPI/IDataItem.h
	inc/MantidAPI/IEventList.h
	inc/MantidAPI/IEventWorkspace.h
	inc/MantidAPI/IFunction.h
	inc/MantidAPI/IFunctionMD.h
	inc/MantidAPI/IFunction1D.h
	inc/MantidAPI/IFunctionMW.h
	inc/MantidAPI/IFunctionWithLocation.h
	inc/MantidAPI/ILiveListener.h
	inc/MantidAPI/ILocatedData.h
	inc/MantidAPI/IMDEventWorkspace.h
	inc/MantidAPI/IMDHistoWorkspace.h
	inc/MantidAPI/IMDIterator.h
	inc/MantidAPI/IMDWorkspace.h
	inc/MantidAPI/IPeak.h
	inc/MantidAPI/IPeakFunction.h
	inc/MantidAPI/IPeaksWorkspace.h
	inc/MantidAPI/ISpectrum.h
	inc/MantidAPI/ITableWorkspace.h
	inc/MantidAPI/IWorkspaceProperty.h
	inc/MantidAPI/ImplicitFunctionBuilder.h
	inc/MantidAPI/ImplicitFunctionFactory.h
	inc/MantidAPI/ImplicitFunctionParameter.h
	inc/MantidAPI/ImplicitFunctionParameterParser.h
	inc/MantidAPI/ImplicitFunctionParameterParserFactory.h
	inc/MantidAPI/ImplicitFunctionParser.h
	inc/MantidAPI/ImplicitFunctionParserFactory.h
	inc/MantidAPI/InstrumentDataService.h
	inc/MantidAPI/Jacobian.h
	inc/MantidAPI/LiveListenerFactory.h
	inc/MantidAPI/LoadAlgorithmFactory.h
	inc/MantidAPI/LocatedDataRef.h
	inc/MantidAPI/LocatedDataValue.h
	inc/MantidAPI/MDGeometry.h
	inc/MantidAPI/MatrixWSIndexCalculator.h
	inc/MantidAPI/MatrixWorkspace.h
	inc/MantidAPI/MatrixWorkspaceMDIterator.h
	inc/MantidAPI/MemoryManager.h
	inc/MantidAPI/MultipleExperimentInfos.h
	inc/MantidAPI/MultipleFileProperty.h
	inc/MantidAPI/NullCoordTransform.h
	inc/MantidAPI/NumericAxis.h
	inc/MantidAPI/ParamFunction.h
	inc/MantidAPI/ParameterReference.h
	inc/MantidAPI/ParameterTie.h
	inc/MantidAPI/Progress.h
	inc/MantidAPI/PropertyNexus.h
	inc/MantidAPI/RefAxis.h
	inc/MantidAPI/Run.h
	inc/MantidAPI/Sample.h
	inc/MantidAPI/SampleEnvironment.h
	inc/MantidAPI/SingleValueParameter.h
	inc/MantidAPI/SingleValueParameterParser.h
	inc/MantidAPI/SpectraAxis.h
	inc/MantidAPI/SpectraDetectorMap.h
	inc/MantidAPI/TableRow.h
	inc/MantidAPI/TextAxis.h
	inc/MantidAPI/VectorParameter.h
	inc/MantidAPI/VectorParameterParser.h
	inc/MantidAPI/Workspace.h
	inc/MantidAPI/WorkspaceFactory.h
	inc/MantidAPI/WorkspaceGroup.h
	inc/MantidAPI/WorkspaceHistory.h
	inc/MantidAPI/WorkspaceIterator.h
	inc/MantidAPI/WorkspaceIteratorCode.h
	inc/MantidAPI/WorkspaceOpOverloads.h
	inc/MantidAPI/WorkspaceProperty.h
	inc/MantidAPI/WorkspaceValidators.h
)

set ( TEST_FILES
	test/AlgorithmHasPropertyTest.h
	test/AlgorithmHistoryTest.h
	test/AlgorithmManagerTest.h
	test/AlgorithmPropertyTest.h
	test/AlgorithmProxyTest.h
	test/AlgorithmTest.h
	test/AnalysisDataServiceTest.h
	test/AsynchronousTest.h
	test/BoxControllerTest.h
	test/CompositeFunctionTest.h
	test/CoordTransformTest.h
	test/CostFunctionFactoryTest.h
	test/EnabledWhenWorkspaceIsTypeTest.h
	test/ExperimentInfoTest.h
	test/ExpressionTest.h
	test/FileFinderTest.h
	test/FilePropertyTest.h
	test/FrameworkManagerTest.h
	test/FunctionPropertyTest.h
	test/FunctionTest.h
	test/IEventListTest.h
	test/IFunction1DTest.h
	test/IFunctionMDTest.h
	test/ILiveListenerTest.h
	test/IMDWorkspaceTest.h
	test/ISpectrumTest.h
	test/ImplicitFunctionParserFactoryTest.h
	test/InstrumentDataServiceTest.h
	test/LiveListenerFactoryTest.h
	test/MDGeometryTest.h
	test/MatrixWorkspaceMDIteratorTest.h
	test/MemoryManagerTest.h
	test/MultipleExperimentInfosTest.h
	test/MultipleFilePropertyTest.h
	test/NumericAxisTest.h
	test/ParameterReferenceTest.h
	test/ParameterTieTest.h
	test/PropertyNexusTest.h
	test/RunTest.h
	test/SampleEnvironmentTest.h
	test/SampleTest.h
	test/SpectraAxisTest.h
	test/SpectraDetectorMapTest.h
	test/TextAxisTest.h
	test/VectorParameterParserTest.h
	test/VectorParameterTest.h
	test/WorkspaceFactoryTest.h
	test/WorkspaceGroupTest.h
	test/WorkspaceHistoryTest.h
	test/WorkspacePropertyTest.h
)

set ( GMOCK_TEST_FILES
        test/ImplicitFunctionFactoryTest.h
        test/ImplicitFunctionParameterParserFactoryTest.h
	test/MatrixWorkspaceTest.h
)


if(UNITY_BUILD)
  include(UnityBuild)
  enable_unity_build(API SRC_FILES SRC_UNITY_IGNORE_FILES 10)
endif(UNITY_BUILD)

# Have to link to winsock library on Windows
if ( WIN32 )
  set ( WINSOCK ws2_32 )
endif ()

# Add a precompiled header where they are supported
ADD_PRECOMPILED_HEADER( inc/MantidAPI/PrecompiledHeader.h MantidAPI src/PrecompiledHeader.cpp SRC_FILES INC_FILES )
# Add the target for this directory
add_library ( API ${SRC_FILES} ${INC_FILES})
# Set the name of the generated library
set_target_properties ( API PROPERTIES OUTPUT_NAME MantidAPI 
                                       COMPILE_DEFINITIONS IN_MANTID_API )
# Add to the 'Framework' group in VS
set_property ( TARGET API PROPERTY FOLDER "MantidFramework" )

target_link_libraries ( API ${MANTIDLIBS} ${WINSOCK} )

if ( CXXTEST_FOUND )
  include_directories ( ../TestHelpers/inc )
  if ( GMOCK_FOUND AND GTEST_FOUND )
    cxxtest_add_test ( APITest ${TEST_FILES} ${GMOCK_TEST_FILES})
    target_link_libraries( APITest API ${GMOCK_LIBRARIES} ${GTEST_LIBRARIES}  )
  else ()
    cxxtest_add_test ( APITest ${TEST_FILES} )
    target_link_libraries( APITest API  )
  endif()
  add_dependencies ( FrameworkTests APITest )
  # Add to the 'FrameworkTests' group in VS
  set_property ( TARGET APITest PROPERTY FOLDER "UnitTests" )
endif ()

###########################################################################
# Installation settings
###########################################################################

install ( TARGETS API DESTINATION ${LIB_DIR} )
