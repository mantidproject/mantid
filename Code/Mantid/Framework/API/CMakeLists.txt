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
	src/CatalogFactory.cpp
	src/CloneableAlgorithm.cpp
	src/Column.cpp
	src/ColumnFactory.cpp
	src/CompositeFunction.cpp
	src/CompositeFunctionMW.cpp
	src/ConstraintFactory.cpp
	src/DeprecatedAlgorithm.cpp
	src/Expression.cpp
	src/FileFinder.cpp
	src/FileProperty.cpp
	src/FrameworkManager.cpp
	src/FunctionFactory.cpp
	src/IDataFileChecker.cpp
	src/IEventWorkspace.cpp
	src/IFitFunction.cpp
	src/IFunctionMW.cpp
	src/IFunctionMD.cpp
	src/IMDIterator.cpp
	src/IMDWorkspace.cpp
	src/IMDEventWorkspace.cpp
	src/ImplicitFunctionParameterParserFactory.cpp
	src/ImplicitFunctionParserFactory.cpp
	src/ImplictFunctionFactory.cpp
	src/InstrumentDataService.cpp
	src/IPeakFunction.cpp
	src/ITableWorkspace.cpp
	src/LoadAlgorithmFactory.cpp
	src/LocatedDataRef.cpp
	src/LocatedDataValue.cpp
	src/MatrixWorkspace.cpp
	src/MatrixWSIndexCalculator.cpp
	src/MDPropertyGeometry.cpp
	src/MemoryManager.cpp
	src/NumericAxis.cpp
	src/PairedGroupAlgorithm.cpp
	src/ParameterReference.cpp
	src/ParameterTie.cpp
	src/ParamFunction.cpp
	src/Progress.cpp
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
	src/WorkspaceTracer.cpp )

set ( INC_FILES 
	inc/MantidAPI/AlgorithmFactory.h
	inc/MantidAPI/Algorithm.h
	inc/MantidAPI/AlgorithmHasProperty.h
	inc/MantidAPI/AlgorithmHistory.h
	inc/MantidAPI/AlgorithmManager.h
	inc/MantidAPI/AlgorithmObserver.h
    inc/MantidAPI/AlgorithmProperty.h
	inc/MantidAPI/AlgorithmProxy.h
	inc/MantidAPI/AnalysisDataService.h
	inc/MantidAPI/ArchiveSearchFactory.h
	inc/MantidAPI/Axis.h
	inc/MantidAPI/CatalogFactory.h
	inc/MantidAPI/CloneableAlgorithm.h
	inc/MantidAPI/ColumnFactory.h
	inc/MantidAPI/Column.h
	inc/MantidAPI/CompositeFunction.h
	inc/MantidAPI/CompositeFunctionMW.h
	inc/MantidAPI/ConstraintFactory.h
	inc/MantidAPI/DeclareUserAlg.h
	inc/MantidAPI/DeprecatedAlgorithm.h
	inc/MantidAPI/DllExport.h
	inc/MantidAPI/Expression.h
	inc/MantidAPI/FileFinder.h
	inc/MantidAPI/FileProperty.h
	inc/MantidAPI/FrameworkManager.h
	inc/MantidAPI/FunctionFactory.h
	inc/MantidAPI/IAlgorithm.h
	inc/MantidAPI/IArchiveSearch.h
	inc/MantidAPI/IBackgroundFunction.h
	inc/MantidAPI/ICatalog.h
	inc/MantidAPI/IConstraint.h
	inc/MantidAPI/IDataFileChecker.h
	inc/MantidAPI/IDataItem.h
	inc/MantidAPI/IEventWorkspace.h
	inc/MantidAPI/IFitFunction.h
	inc/MantidAPI/IFunctionMW.h
	inc/MantidAPI/IFunctionMD.h
	inc/MantidAPI/IMDIterator.h
	inc/MantidAPI/IFunctionWithLocation.h
	inc/MantidAPI/ILocatedData.h
	inc/MantidAPI/IMDEventWorkspace.h
	inc/MantidAPI/IMDWorkspace.h
	inc/MantidAPI/ImplicitFunctionBuilder.h
	inc/MantidAPI/ImplicitFunctionFactory.h
	inc/MantidAPI/ImplicitFunction.h
	inc/MantidAPI/ImplicitFunctionParameter.h
	inc/MantidAPI/ImplicitFunctionParameterParserFactory.h
	inc/MantidAPI/ImplicitFunctionParameterParser.h
	inc/MantidAPI/ImplicitFunctionParserFactory.h
	inc/MantidAPI/ImplicitFunctionParser.h
	inc/MantidAPI/InstrumentDataService.h
	inc/MantidAPI/IPeakFunction.h
	inc/MantidAPI/ITableWorkspace.h
	inc/MantidAPI/IWorkspaceProperty.h
	inc/MantidAPI/LoadAlgorithmFactory.h
	inc/MantidAPI/LocatedDataRef.h
	inc/MantidAPI/LocatedDataValue.h
	inc/MantidAPI/MatrixWorkspace.h
	inc/MantidAPI/MatrixWSIndexCalculator.h
	inc/MantidAPI/MDPropertyGeometry.h
	inc/MantidAPI/MemoryManager.h
	inc/MantidAPI/NumericAxis.h
	inc/MantidAPI/PairedGroupAlgorithm.h
	inc/MantidAPI/ParameterReference.h
	inc/MantidAPI/ParameterTie.h
	inc/MantidAPI/ParamFunction.h
	inc/MantidAPI/Point3D.h
	inc/MantidAPI/Progress.h
	inc/MantidAPI/RefAxis.h
	inc/MantidAPI/Run.h
	inc/MantidAPI/SampleEnvironment.h
	inc/MantidAPI/Sample.h
	inc/MantidAPI/ScriptWriter.h
	inc/MantidAPI/ScriptWriterFactory.h
	inc/MantidAPI/SpectraAxis.h
	inc/MantidAPI/SpectraDetectorMap.h
	inc/MantidAPI/TableRow.h
	inc/MantidAPI/TextAxis.h
	inc/MantidAPI/WorkspaceFactory.h
	inc/MantidAPI/WorkspaceGroup.h
	inc/MantidAPI/Workspace.h
	inc/MantidAPI/WorkspaceHistory.h
	inc/MantidAPI/WorkspaceIteratorCode.h
	inc/MantidAPI/WorkspaceIterator.h
	inc/MantidAPI/WorkspaceOpOverloads.h
	inc/MantidAPI/WorkspaceProperty.h
	inc/MantidAPI/WorkspaceTracer.h
	inc/MantidAPI/WorkspaceValidators.h )

set ( TEST_FILES 
        test/AlgorithmHasPropertyTest.h
	test/AlgorithmHistoryTest.h
	test/AlgorithmManagerTest.h
        test/AlgorithmPropertyTest.h
	test/AlgorithmProxyTest.h
	test/AlgorithmTest.h
	test/AnalysisDataServiceTest.h
	test/AsynchronousTest.h
	test/AxisTest.h
	test/CompositeFunctionTest.h
	test/ExpressionTest.h
	test/FileFinderTest.h
	test/FilePropertyTest.h
	test/FrameworkManagerTest.h
	test/FunctionTest.h
	test/IFunctionMDTest.h
	test/IMDWorkspaceTest.h
	test/ImplicitFunctionParserFactoryTest.h
	test/InstrumentDataServiceTest.h
	test/MatrixWorkspaceTest.h
	test/MDPropertyGeometryTest.h
	test/MemoryManagerTest.h
	test/ParameterReferenceTest.h
	test/ParameterTieTest.h
	test/RunTest.h
	test/SampleEnvironmentTest.h
	test/SampleTest.h
	test/ScriptWriterTest.h
	test/ScriptWriterFactoryTest.h
	test/SpectraDetectorMapTest.h
	test/TextAxisTest.h
	test/WorkspaceFactoryTest.h
	test/WorkspacePropertyTest.h )

set ( GMOCK_TEST_FILES
	test/ImplicitFunctionFactoryTest.h
	test/ImplicitFunctionParameterParserFactoryTest.h
)

# Have to link to winsock library on Windows
if ( WIN32 )
  set ( WINSOCK ws2_32 )
endif ()

# Add the target for this directory
add_library ( API ${SRC_FILES} ${INC_FILES})
# Set the name of the generated library
set_target_properties ( API PROPERTIES OUTPUT_NAME MantidAPI 
                                       COMPILE_DEFINITIONS IN_MANTID_API )
# Add to the 'Framework' group in VS
set_property ( TARGET API PROPERTY FOLDER "MantidFramework" )

target_link_libraries ( API ${MANTIDLIBS} ${WINSOCK} )

if ( CXXTEST_FOUND )
  include_directories ( ../TestHelpers/inc ../DataObjects/inc )
  if ( GMOCK_FOUND AND GTEST_FOUND )
    cxxtest_add_test ( APITest ${TEST_FILES} ${GMOCK_TEST_FILES})
    target_link_libraries( APITest API ${GMOCK_LIBRARIES} TestHelpers )
  else ()
    cxxtest_add_test ( APITest ${TEST_FILES} )
    target_link_libraries( APITest API TestHelpers )
  endif()
  add_dependencies ( FrameworkTests APITest )
  # Add to the 'FrameworkTests' group in VS
  set_property ( TARGET APITest PROPERTY FOLDER "UnitTests" )
endif ()

###########################################################################
# Installation settings
###########################################################################

install ( TARGETS API DESTINATION ${LIB_DIR} )
