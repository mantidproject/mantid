# System tests related to the SANS2D instrument at ISIS.

set(SYSTEST_NAMES
    SANS2D_empty_cycle_22_02.py
    SANS2D_GDW20_4m_cycle_22_02.py
    SANS2D_GDW20_12m_cycle_22_02.py
    SANS2D_glassy_carbon_cycle_22_02.py
    SANS2DBatchTest.py
    SANS2DFrontNoGrav.py
    SANS2DLimitEventsTime.py
    SANS2DMultiPeriod.py
    SANS2DMultiPeriodAddFilesTest.py
    SANS2DReductionGUI.py
    SANS2DReductionGUIAddedTest.py
    SANS2DReductionGUITest.py
    SANS2DSlicingTest.py
    SANS2DTubeCalibrationTest.py
    SANS2DTubeMergeTest.py
    SANS2DWaveloopsTest.py
)

pysystemtest_add_test(${CMAKE_CURRENT_SOURCE_DIR} framework.ISIS.SANS.SANS2D ${SYSTEST_NAMES})
