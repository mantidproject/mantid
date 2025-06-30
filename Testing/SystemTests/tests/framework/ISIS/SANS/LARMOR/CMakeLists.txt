# System tests related to the LARMOR instrument at ISIS.

set(SYSTEST_NAMES SANSLARMOR_multiperiod_event_files_TOML.py SANSLARMORMultiPeriod SANSLARMORMultiPeriod_V2.py
                  SANSLARMORMultiPeriodAddFiles.py SANSLARMORMultiPeriodAddFilesTest_V2.py
)

pysystemtest_add_test(${SYSTEST_NAMES})
