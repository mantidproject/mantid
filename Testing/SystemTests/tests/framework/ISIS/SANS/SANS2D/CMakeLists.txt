# System tests related to the SANS2D instrument at ISIS.

set(SYSTEST_NAMES
    SANS2D_empty_cycle_22_02
    SANS2D_GDW20_4m_cycle_22_02
    SANS2D_GDW20_12m_cycle_22_02
    SANS2D_glassy_carbon_cycle_22_02
    SANS2DBatch
    SANS2DBatchTest_V2
    SANS2DFrontNoGrav
    SANS2DFrontNoGrav_V2
    SANS2DLimitEventsTime
    SANS2DLimitEventsTime_V2
    SANS2DMultiPeriod
    SANS2DMultiPeriod_V2
    SANS2DMultiPeriodAddFiles
    SANS2DMultiPeriodAddFilesTest_V2
    SANS2DReductionGUI
    SANS2DReductionGUI_V2
    SANS2DReductionGUIAdded
    SANS2DReductionGUIAddedTest_V2
    SANS2DReductionGUITest_V2
    SANS2DReloadWorkspaces
    SANS2DSearchCentreGUI
    SANS2DSlicing
    SANS2DSlicingTest_V2
    SANS2DTubeCalibrationTest
    SANS2DTubeMergeTest
    SANS2DWaveloops
    SANS2DWaveloopsTest_V2
)

pysystemtest_add_test(${SYSTEST_NAMES})
