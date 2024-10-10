# mantid.api tests

set(TEST_PY_FILES
    ADSValidatorTest.py
    AlgorithmTest.py
    AlgorithmFactoryTest.py
    AlgorithmFactoryObserverTest.py
    AlgorithmHistoryTest.py
    AlgorithmManagerTest.py
    AlgorithmPropertyTest.py
    AlgoRegisterTest.py
    AnalysisDataServiceTest.py
    AnalysisDataServiceObserverTest.py
    AxisTest.py
    CatalogManagerTest.py
    CompositeFunctionTest.py
    DataProcessorAlgorithmTest.py
    DeprecatedAlgorithmCheckerTest.py
    ExperimentInfoTest.py
    FilePropertyTest.py
    FileFinderTest.py
    FrameworkManagerTest.py
    FunctionFactoryTest.py
    FunctionPropertyTest.py
    IEventWorkspaceTest.py
    IFunction1DTest.py
    IFunction1DValidatorTest.py
    IMaskWorkspaceTest.py
    IPeakFunctionTest.py
    IPeaksWorkspaceTest.py
    ITableWorkspaceTest.py
    JacobianTest.py
    MatrixWorkspaceTest.py
    MDEventWorkspaceTest.py
    MDGeometryTest.py
    MDHistoWorkspaceTest.py
    MultipleExperimentInfos.py
    MultipleFilePropertyTest.py
    PreviewManagerTest.py
    ProgressTest.py
    ProjectionTest.py
    PythonAlgorithmPropertiesTest.py
    PythonAlgorithmChildAlgCallTest.py
    PythonAlgorithmTraitsTest.py
    PythonAlgorithmWorkspacePropertyTest.py
    RunPythonScriptTest.py
    RunTest.py
    SampleTest.py
    SpectrumInfoTest.py
    WorkspaceBinaryOpsTest.py
    WorkspaceFactoryTest.py
    WorkspaceTest.py
    WorkspaceGroupTest.py
    WorkspaceHistoryTest.py
    WorkspacePropertiesTest.py
    WorkspaceUnaryOpsTest.py
    WorkspaceValidatorsTest.py
)

check_tests_valid(${CMAKE_CURRENT_SOURCE_DIR} ${TEST_PY_FILES})

# Prefix for test=PythonInterfaceAPI
pyunittest_add_test(${CMAKE_CURRENT_SOURCE_DIR} python.api ${TEST_PY_FILES})
