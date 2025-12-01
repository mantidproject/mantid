# System tests related to the SANS2D instrument at ISIS.

set(SYSTEST_NAMES
    SANS2D_empty_cycle_22_02.py
    SANS2D_GDW20_4m_cycle_22_02.py
    SANS2D_GDW20_12m_cycle_22_02.py
    SANS2D_glassy_carbon_cycle_22_02.py
    SANS2DBatch.py
    SANS2DBatchTest_V2.py
    SANS2DFrontNoGrav.py
    SANS2DFrontNoGrav_V2.py
    SANS2DLimitEventsTime.py
    SANS2DLimitEventsTime_V2.py
    SANS2DMultiPeriod.py
    SANS2DMultiPeriod_V2.py
    SANS2DMultiPeriodAddFiles.py
    SANS2DMultiPeriodAddFilesTest_V2.py
    SANS2DReductionGUI.py
    SANS2DReductionGUI_V2.py
    SANS2DReductionGUIAdded.py
    SANS2DReductionGUIAddedTest_V2.py
    SANS2DReductionGUITest_V2.py
    SANS2DReloadWorkspaces.py
    SANS2DSearchCentreGUI.py
    SANS2DSlicing.py
    SANS2DSlicingTest_V2.py
    SANS2DTubeCalibrationTest.py
    SANS2DTubeMergeTest.py
    SANS2DWaveloops.py
    SANS2DWaveloopsTest_V2.py
)

pysystemtest_add_test(${CMAKE_CURRENT_SOURCE_DIR} framework.ISIS.SANS.SANS2D ${SYSTEST_NAMES})
