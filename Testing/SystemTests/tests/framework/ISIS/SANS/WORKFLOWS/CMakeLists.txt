# System tests related to the SANS Workflows.

set(SYSTEST_NAMES
    SANSBatchReductionTest.py
    SANSBeamCentreFinderCoreTest.py
    SANSCentreSample.py
    SANSDarkRunSubtractionTest.py
    SANSFileChecking.py
    SANSLoadTest.py
    SANSMergedDetectorsTest.py
    SANSQResolutionTest.py
    SANSReductionCoreTest.py
    SANSSaveTest.py
    SANSSingleReductionTest.py
    SANSUtilityTest.py
    SANSWorkspaceTypeTest.py
)

pysystemtest_add_test(${CMAKE_CURRENT_SOURCE_DIR} framework.ISIS.SANS.WORKFLOWS ${SYSTEST_NAMES})
