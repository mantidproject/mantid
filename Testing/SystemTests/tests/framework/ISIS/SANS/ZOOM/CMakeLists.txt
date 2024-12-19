# System tests related to the ZOOM instrument at ISIS.

set(SYSTEST_NAMES
    SANSZOOMLegacyUserFileTest
    SANSZOOMTomlFileConversion
    SANSZOOMTomlFileTest
    SANSZOOMWavLoopsTest
    ZOOMLegacyUserFileTest
    ZOOMTomlFileTest
    ZOOMTransEventData
    ZOOMWavLoopsTest
)

pysystemtest_add_test(${SYSTEST_NAMES})
