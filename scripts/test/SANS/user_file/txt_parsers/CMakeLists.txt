set(TEST_PY_FILES
    ParserAdapterTest.py
    ParserAdapterBackgroundDetailsTest.py
    ParserAdapterDetectorDetailsTest.py
    ParserAdapterFitDetailsTest.py
    ParserAdapterLimitDetailsTest.py
    ParserAdapterMaskDetailsTest.py
    ParserAdapterMonitorDetailsTest.py
    ParserAdapterQResolutionDetailsTest.py
    ParserAdapterSampleDetailsTest.py
    ParserAdapterSetPositionDetailsTest.py
    ParserAdapterTransmissionDetailsTest.py
)

check_tests_valid(${CMAKE_CURRENT_SOURCE_DIR} ${TEST_PY_FILES})

pyunittest_add_test(${CMAKE_CURRENT_SOURCE_DIR} python.SANS.user_file.txt_parsers ${TEST_PY_FILES})
