set(TEST_PY_FILES
    ParsedDictConverterTest.py
    ParsedDictConverterBackgroundDetailsTest.py
    ParsedDictConverterDetectorDetailsTest.py
    ParsedDictConverterFitDetailsTest.py
    ParsedDictConverterLimitDetailsTest.py
    ParsedDictConverterMaskDetailsTest.py
    ParsedDictConverterMonitorDetailsTest.py
    ParsedDictConverterQResolutionDetailsTest.py
    ParsedDictConverterSampleDetailsTest.py
    ParsedDictConverterSetPositionDetailsTest.py
    ParsedDictConverterTransmissionDetailsTest.py
    UserFileParserAdapterTest.py
)

check_tests_valid(${CMAKE_CURRENT_SOURCE_DIR} ${TEST_PY_FILES})

pyunittest_add_test(${CMAKE_CURRENT_SOURCE_DIR} python.SANS.user_file.txt_parsers ${TEST_PY_FILES})
