# Automated UI tests for the Engineering Diffraction interface, replacing
# dev-docs/source/Testing/EngineeringDiffraction/EngineeringDiffractionTestGuide.rst.
#
# eng_diff_gui_test_base.py is not listed: it holds the shared setup and the synthetic data fixture, not tests. Its
# classes define no test_* method, so the unittest loader ignores them even in the modules that import them.
set(TEST_NAMES
    EngDiffGuiCorrectionTest.py
    EngDiffGuiCroppingTest.py
    EngDiffGuiFittingTest.py
    EngDiffGuiGsas2Test.py
    EngDiffGuiImatTest.py
    EngDiffGuiRunProcessingTest.py
    EngDiffGuiTextureTest.py
)

# The parent's PYUNITTEST_PYTHONPATH_EXTRA (the shared harness directory) is inherited here, and PY_ADD_TEST puts this
# directory on PYTHONPATH itself, so both eng_diff_gui_test_base and the cross-interface helpers are importable from the
# test modules without adding anything further.
pyunittest_add_test_ui(${CMAKE_CURRENT_SOURCE_DIR} AutomatedUITest.EngineeringDiffraction ${TEST_NAMES})
