# System tests related to the SANS Workflows.

set(SYSTEST_NAMES
    SANSBatchReductionTest
    SANSBeamCentreFinderCoreTest
    SANSCentreSample
    SANSDarkRunSubtractionTest
    SANSDiagnosticPageTest
    SANSFileChecking
    SANSLoadersTest
    SANSLoadTest
    SANSLOWCentreNoGrav_V2
    SANSMergedDetectorsTest
    SANSMergedDetectorsTest_V2
    SANSQResolutionTest
    SANSReductionCoreTest
    SANSSaveTest
    SANSSingleReductionTest
    SANSUtilityTest
    SANSWorkspaceTypeTest
)

pysystemtest_add_test(${SYSTEST_NAMES})
