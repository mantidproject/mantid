##
## mantid.api tests
##
set ( TEST_PY_FILES
  AlgorithmTest.py
  AlgorithmFactoryTest.py
  AlgorithmHistoryTest.py
  AlgorithmManagerTest.py
  AlgorithmPropertyTest.py
  AnalysisDataServiceTest.py
  AxisTest.py
  CatalogManagerTest.py
  CutMDTest.py
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
  IPeakFunctionTest.py
  IPeaksWorkspaceTest.py
  ITableWorkspaceTest.py
  JacobianTest.py
  MatrixWorkspaceTest.py
  MDGeometryTest.py
  MDHistoWorkspaceTest.py
  MultipleExperimentInfos.py
  MultipleFilePropertyTest.py
  ProgressTest.py
  ProjectionTest.py
  PythonAlgorithmPropertiesTest.py
  PythonAlgorithmChildAlgCallTest.py
  PythonAlgorithmTraitsTest.py
  PythonAlgorithmWorkspacePropertyTest.py
  RunPythonScriptTest.py
  RunTest.py
  SampleTest.py
  WorkspaceFactoryTest.py
  WorkspaceTest.py
  WorkspaceGroupTest.py
  WorkspaceHistoryTest.py
  WorkspacePropertiesTest.py
  WorkspaceValidatorsTest.py
)

check_tests_valid ( ${CMAKE_CURRENT_SOURCE_DIR} ${TEST_PY_FILES} )

# Prefix for test=PythonInterfaceAPI
pyunittest_add_test ( ${CMAKE_CURRENT_SOURCE_DIR} PythonInterfaceAPI ${TEST_PY_FILES} )
