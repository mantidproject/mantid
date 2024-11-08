# System Tests for the mantid qt package

set(SYSTEST_NAMES CppInterfacesStartupTest FitScriptGeneratorStartupTest PythonInterfacesStartupTest
                  SliceViewerIntegrationTest WorkbenchStartupTest
)

if(ENABLE_WORKBENCH)
  pysystemtest_add_test(${SYSTEST_NAMES})
endif()
