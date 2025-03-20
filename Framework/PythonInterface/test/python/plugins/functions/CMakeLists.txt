# Tests for Fit functions

set(TEST_PY_FILES
    AFM_LFTest.py
    AFM_ZFTest.py
    AttributeTest.py
    BesselTest.py
    ChudleyElliotTest.py
    CombGaussLorentzKTTest.py
    DampedBesselTest.py
    CompositePCRmagnetTest.py
    EISFDiffSphereTest.py
    EISFDiffCylinderTest.py
    EISFDiffSphereAlkylTest.py
    Example1DFunctionTest.py
    ExamplePeakFunctionTest.py
    FlatTopPeakTest.py
    FmuFTest.py
    GauBroadGauKTTest.py
    GaussBesselTest.py
    HighTFMuoniumTest.py
    LowTFMuoniumTest.py
    ModOscTest.py
    MsdGaussTest.py
    MsdPetersTest.py
    MsdYiTest.py
    MuHTest.py
    MuMinusExpTFTest.py
    PCRmagnetTest.py
    PCRmagnetZFKTTest.py
    PCRmagnetRedfieldTest.py
    PCRmagnetfnormTest.py
    PEARLTransVoigtTest.py
    PearsonIVTest.py
    PrimStretchedExpFTTest.py
    RedfieldTest.py
    RFresonanceTest.py
    SpinDiffusionTest.py
    SpinGlassTest.py
    StandardSCTest.py
    StaticLorentzianKTTest.py
    StretchedExpFTTest.py
    StretchedKTTest.py
    TeixeiraWaterTest.py
    TeixeiraWaterIqtTest.py
    TeixeiraWaterIqtFTTest.py
    TFMuoniumTest.py
    ZFprotonDipoleTest.py
    ZFdipoleTest.py
    ZFelectronDipoleTest.py
    ZFMuoniumTest.py
)

check_tests_valid(${CMAKE_CURRENT_SOURCE_DIR} ${TEST_PY_FILES})

# Prefix for test name=PythonFunctions
pyunittest_add_test(${CMAKE_CURRENT_SOURCE_DIR} python.functions ${TEST_PY_FILES})
