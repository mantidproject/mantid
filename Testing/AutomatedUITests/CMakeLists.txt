# Automated UI tests: tests that drive a real Qt interface with QTest, replacing the manual test guides in
# dev-docs/source/Testing.
#
# They live outside Testing/SystemTests on purpose. runSystemTests.py collects by walking the "framework" and "qt"
# directories, so a suite under either of those is picked up by the nightly system test run whatever CTest label it
# carries. From here, the AutomatedUITest label given by pyunittest_add_test_ui is the only way in, and neither the pull
# request build (ctest -L UnitTest) nor the nightly selects it. See dev-docs/source/Testing/AutomatedUITests.rst.
#
# No tests are registered at this level - each interface gets its own subdirectory. The shared harness modules here
# (automated_ui_test_base.py, qt_interaction_helpers.py) are deliberately not listed as tests; putting this directory on
# the test PYTHONPATH is what makes them importable from the interface subdirectories, and CMake variables propagate
# into add_subdirectory.
set(PYUNITTEST_PYTHONPATH_EXTRA ${CMAKE_CURRENT_SOURCE_DIR})
set(PYUNITTEST_QT_API pyqt${MANTID_QT_VERSION}) # force to use the configured Qt version
# these drive a real interface and run real algorithms, so they are both slow and greedy: give them a longer limit than
# the global one and keep them off each other's toes
set(PYUNITTEST_TIMEOUT 3600)
set(PYUNITTEST_RUN_SERIAL ON)

# A UI test replaces a manual test guide, so it uses whatever data that guide told a tester to load - which for most
# interfaces means the SystemTest set. The built properties file only puts UnitTest and DocTest on the search path, so
# name the rest here and let the harness add them per test. Their contents are downloaded by the SystemTestData and
# StandardTestData targets; a test whose files are absent reports a skip through require_files.
set(PYUNITTEST_DATA_DIRS
    ${ExternalData_BINARY_ROOT}/Testing/Data/SystemTest ${ExternalData_BINARY_ROOT}/Testing/Data/DocTest
    ${ExternalData_BINARY_ROOT}/Testing/Data/UnitTest
)

add_subdirectory(Example)
