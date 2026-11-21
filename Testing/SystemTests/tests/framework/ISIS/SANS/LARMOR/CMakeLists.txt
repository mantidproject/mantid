# System tests related to the LARMOR instrument at ISIS.

set(SYSTEST_NAMES SANSLARMOR_multiperiod_event_files_TOML.py SANSLARMORMultiPeriod.py
                  SANSLARMORMultiPeriodAddFilesTest.py
)

pysystemtest_add_test(${CMAKE_CURRENT_SOURCE_DIR} framework.ISIS.SANS.LARMOR ${SYSTEST_NAMES})

set_tests_properties(
  framework.ISIS.SANS.LARMOR.SANSLARMORMultiPeriodAddFilesTest.SANSLARMORMultiPeriodAddFilesTest
  framework.ISIS.SANS.LARMOR.SANSLARMOR_multiperiod_event_files_TOML.SANSLARMOR_multiperiod_event_files_TOML
  PROPERTIES RESOURCE_LOCK "SANS_LARMOR_13065_ADD"
)
