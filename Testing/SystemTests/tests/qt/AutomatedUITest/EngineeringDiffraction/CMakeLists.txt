# Automated UI tests for the Engineering Diffraction interface.
#
# eng_diff_gui_test_base.py is not listed: it holds the shared setup and the synthetic data fixture, not tests. Its base
# class is abstract so the system test collector ignores it even in the modules that import it.
set(SYSTEST_NAMES
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
pysystemtest_add_test(${CMAKE_CURRENT_SOURCE_DIR} qt.AutomatedUITest.EngineeringDiffraction ${SYSTEST_NAMES})
