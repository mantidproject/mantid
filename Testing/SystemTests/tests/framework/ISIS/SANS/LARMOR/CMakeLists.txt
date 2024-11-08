# System tests related to the LARMOR instrument at ISIS.

set(SYSTEST_NAMES SANSLARMOR_multiperiod_event_files_TOML SANSLARMORMultiPeriod SANSLARMORMultiPeriod_V2
                  SANSLARMORMultiPeriodAddFiles SANSLARMORMultiPeriodAddFilesTest_V2
)

pysystemtest_add_test(${SYSTEST_NAMES})
