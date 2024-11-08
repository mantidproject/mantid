# System tests related to the LOQ instrument at ISIS.

set(SYSTEST_NAMES
    LOQReloadWorkspaces
    SANSLOQAddBatch
    SANSLOQBatch
    SANSLOQBatch_V2
    SANSLOQCan2D
    SANSLOQCan2D_TOML
    SANSLOQCan2D_V2
    SANSLOQCentreNoGrav
    SANSLOQLegacyUserFile
    SANSLOQReductionGUI
    SANSLOQReductionGUITest_V2
    SANSLOQTransFitWorkspace2D
)

pysystemtest_add_test(${SYSTEST_NAMES})
