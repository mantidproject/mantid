# System tests related to the LOQ instrument at ISIS.

set(SYSTEST_NAMES
    SANSLOQAddBatch.py
    SANSLOQBatch.py
    SANSLOQCan2D.py
    SANSLOQCan2D_TOML.py
    SANSLOQCentreNoGrav.py
    SANSLOQLegacyUserFile.py
    SANSLOQReductionGUITest.py
    SANSLOQTransFitWorkspace2D.py
)

pysystemtest_add_test(${CMAKE_CURRENT_SOURCE_DIR} framework.ISIS.SANS.LOQ ${SYSTEST_NAMES})
