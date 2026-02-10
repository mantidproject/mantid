# System Tests for the mantid qt package

set(SYSTEST_NAMES CppInterfacesStartupTest.py FitScriptGeneratorStartupTest.py PythonInterfacesStartupTest.py
                  SANSDiagnosticPageTest.py SliceViewerIntegrationTest.py WorkbenchStartupTest.py
)

if(ENABLE_WORKBENCH)
  pysystemtest_add_test(${CMAKE_CURRENT_SOURCE_DIR} qt ${SYSTEST_NAMES})
endif()
