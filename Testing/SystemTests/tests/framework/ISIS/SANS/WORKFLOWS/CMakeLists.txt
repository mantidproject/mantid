# System tests related to the SANS Workflows.

set(SYSTEST_NAMES
    SANSBatchReductionTest.py
    SANSBeamCentreFinderCoreTest.py
    SANSCentreSample.py
    SANSDarkRunSubtractionTest.py
    SANSFileChecking.py
    SANSLoadersTest.py
    SANSLoadTest.py
    SANSLOWCentreNoGrav_V2.py
    SANSMergedDetectorsTest.py
    SANSMergedDetectorsTest_V2.py
    SANSQResolutionTest.py
    SANSReductionCoreTest.py
    SANSSaveTest.py
    SANSSingleReductionTest.py
    SANSUtilityTest.py
    SANSWorkspaceTypeTest.py
)

pysystemtest_add_test(${CMAKE_CURRENT_SOURCE_DIR} framework.ISIS.SANS.WORKFLOWS ${SYSTEST_NAMES})
