##
## mantid.api tests
##
set ( TEST_PY_FILES
  AlgorithmTest.py
  AlgorithmFactoryTest.py
  AlgorithmManagerTest.py
  AnalysisDataServiceTest.py
  AxisTest.py
  DeprecatedAlgorithmCheckerTest.py
  ExperimentInfoTest.py
  FilePropertyTest.py
  FileFinderTest.py
  FrameworkManagerTest.py
  FunctionFactoryTest.py
  FunctionPropertyTest.py
  IEventWorkspaceTest.py
  IPeaksWorkspaceTest.py
  ITableWorkspaceTest.py
  MatrixWorkspaceTest.py
  MDHistoWorkspaceTest.py
  MultipleExperimentInfos.py
  MultipleFilePropertyTest.py
  ProgressTest.py
  PythonAlgorithmPropertiesTest.py
  PythonAlgorithmChildAlgCallTest.py
  PythonAlgorithmTraitsTest.py
  PythonAlgorithmWorkspacePropertyTest.py
  RunTest.py
  WorkspaceFactoryTest.py
  WorkspaceTest.py
  WorkspaceGroupTest.py
  WorkspacePropertiesTest.py
  WorkspaceValidatorsTest.py
)

# Prefix for test=PythonInterfaceAPI
pyunittest_add_test ( ${CMAKE_CURRENT_SOURCE_DIR} PythonInterfaceAPI ${TEST_PY_FILES} )
