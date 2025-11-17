# System tests related to the ZOOM instrument at ISIS.

set(SYSTEST_NAMES
    SANSZOOMLegacyUserFileTest.py
    SANSZOOMTomlFileConversion.py
    SANSZOOMTomlFileTest.py
    SANSZOOMWavLoopsTest.py
    ZOOMLegacyUserFileTest.py
    ZOOMTomlFileTest.py
    ZOOMTransEventData.py
    ZOOMWavLoopsTest.py
)

pysystemtest_add_test(${CMAKE_CURRENT_SOURCE_DIR} framework.ISIS.SANS.ZOOM ${SYSTEST_NAMES})
