# System tests related to the LOQ instrument at ISIS.

set(SYSTEST_NAMES
    LOQReloadWorkspaces.py
    SANSLOQAddBatch.py
    SANSLOQBatch.py
    SANSLOQBatch_V2.py
    SANSLOQCan2D.py
    SANSLOQCan2D_TOML.py
    SANSLOQCan2D_V2.py
    SANSLOQCentreNoGrav.py
    SANSLOQLegacyUserFile.py
    SANSLOQReductionGUI.py
    SANSLOQReductionGUITest_V2.py
    SANSLOQTransFitWorkspace2D.py
)

pysystemtest_add_test(${CMAKE_CURRENT_SOURCE_DIR} framework.ISIS.SANS.LOQ ${SYSTEST_NAMES})
