# Tests for Fit functions

set(TEST_PY_FILES
    AttributeTest.py
    BesselTest.py
    CombGaussLorentzKTTest.py
    DSFinterp1DFitTest.py
    EISFDiffSphereTest.py
    EISFDiffCylinderTest.py
    EISFDiffSphereAlkylTest.py
    Example1DFunctionTest.py
    ExamplePeakFunctionTest.py
    FmuFTest.py
    HighTFMuoniumTest.py
    MeierTest.py
    MsdGaussTest.py
    MsdPetersTest.py
    MsdYiTest.py
    MuHTest.py
    RFresonanceTest.py
    SilverBaselineTest.py
    SpinGlassTest.py
    StandardSCTest.py
    StaticLorentzianKTTest.py
    StretchedExpFTTest.py
    StretchedKTTest.py
    TeixeiraWaterTest.py
    TriangleOscTest.py
    TFMuoniumTest.py)

check_tests_valid(${CMAKE_CURRENT_SOURCE_DIR} ${TEST_PY_FILES})

# Prefix for test name=PythonFunctions
pyunittest_add_test(${CMAKE_CURRENT_SOURCE_DIR} python.functions
                    ${TEST_PY_FILES})
