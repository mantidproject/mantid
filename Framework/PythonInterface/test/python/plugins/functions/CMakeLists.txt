# Tests for Fit functions

set(TEST_PY_FILES
    AFM_LFTest.py
    AFM_ZFTest.py
    AttributeTest.py
    BesselTest.py
    CombGaussLorentzKTTest.py
    DampedBesselTest.py
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
    PCRmagnetTest.py
    PCRmagnetZFKTTest.py
    PCRmagnetRedfieldTest.py
    PCRmagnetfnormTest.py
    RFresonanceTest.py
    SilverBaselineTest.py
    SpinGlassTest.py
    StandardSCTest.py
    StaticLorentzianKTTest.py
    StretchedExpFTTest.py
    StretchedKTTest.py
    SCgapAnisoSwaveTest.py
    SCgapDwaveTest.py
    SCgapPwaveTest.py
    ScgapSwaveTest.py
    TeixeiraWaterTest.py
    TFMuoniumTest.py
    TriangleOscTest.py
    ZFMuoniumTest.py
    TFMuoniumTest.py
    ZFdipoleTest.py
    ZFprotonDipoleTest.py
    ZFelectronDipoleTest.py)

check_tests_valid(${CMAKE_CURRENT_SOURCE_DIR} ${TEST_PY_FILES})

# Prefix for test name=PythonFunctions
pyunittest_add_test(${CMAKE_CURRENT_SOURCE_DIR} python.functions
                    ${TEST_PY_FILES})
