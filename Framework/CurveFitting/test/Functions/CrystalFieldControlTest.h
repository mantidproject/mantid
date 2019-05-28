// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef CRYSTALFIELDCONTROLTEST_H_
#define CRYSTALFIELDCONTROLTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/Functions/CrystalFieldControl.h"

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::CurveFitting;
using namespace Mantid::CurveFitting::Functions;

class CrystalFieldControlTest : public CxxTest::TestSuite {
public:
  void test_defaults() {
    Mantid::CurveFitting::Functions::CrystalFieldControl cf;
    cf.setAttributeValue("Ions", "Ce");
    cf.setAttributeValue("Symmetries", "C2v");
    cf.setAttributeValue("Temperatures", std::vector<double>({44}));
    cf.setAttributeValue("FWHMs", std::vector<double>({1}));
    std::string Ions = cf.getAttribute("Ions").asString();
    TS_ASSERT_EQUALS(Ions, "\"Ce\"");
    std::string Symmetries = cf.getAttribute("Symmetries").asString();
    TS_ASSERT_EQUALS(Symmetries, "\"C2v\"");
    std::string PhysicalProperties =
        cf.getAttribute("PhysicalProperties").asString();
    TS_ASSERT_EQUALS(PhysicalProperties, "");
    std::string Background = cf.getAttribute("Background").asString();
    TS_ASSERT_EQUALS(Background, "\"\"");
    std::string PeakShape = cf.getAttribute("PeakShape").asString();
    TS_ASSERT_EQUALS(PeakShape, "Lorentzian");
    double FWHMVariation = cf.getAttribute("FWHMVariation").asDouble();
    TS_ASSERT_EQUALS(FWHMVariation, 1.0000000000000001e-01);
    std::vector<double> FWHMs = cf.getAttribute("FWHMs").asVector();
    TS_ASSERT_EQUALS(FWHMs.size(), 1);
    TS_ASSERT_EQUALS(FWHMs[0], 1.0000000000000000e+00);

    std::vector<double> f0_FWHMX = cf.getAttribute("FWHMX").asVector();
    TS_ASSERT(f0_FWHMX.empty());

    std::vector<double> f0_FWHMY = cf.getAttribute("FWHMY").asVector();
    TS_ASSERT(f0_FWHMY.empty());
  }

  void test_build_1() {
    Mantid::CurveFitting::Functions::CrystalFieldControl cf;
    cf.setAttributeValue("Ions", "Ce");
    cf.setAttributeValue("Symmetries", " C2v");
    cf.setAttributeValue("PhysicalProperties", "");
    cf.setAttributeValue("Temperatures", std::vector<double>({44}));
    cf.setAttributeValue("FWHMs", std::vector<double>({1}));
    auto source = cf.buildSource();
    std::string Ions = cf.getAttribute("Ions").asString();
    std::string Symmetries = cf.getAttribute("Symmetries").asString();
    std::string PhysicalProperties =
        cf.getAttribute("PhysicalProperties").asString();
    bool isComposite =
        dynamic_cast<Mantid::API::CompositeFunction *>(source.get()) != nullptr;
    bool isMultiSite = cf.isMultiSite();
    bool isMultiSpectrum = cf.isMultiSpectrum();
    int nControls = (int)cf.nFunctions();
    TS_ASSERT_EQUALS(Ions, "\"Ce\"");
    TS_ASSERT_EQUALS(Symmetries, "\"C2v\"");
    TS_ASSERT_EQUALS(PhysicalProperties, "");
    TS_ASSERT_EQUALS(isComposite, false);
    TS_ASSERT_EQUALS(isMultiSite, false);
    TS_ASSERT_EQUALS(isMultiSpectrum, false);
    TS_ASSERT_EQUALS(nControls, 0);
  }

  void test_build_2() {
    Mantid::CurveFitting::Functions::CrystalFieldControl cf;
    cf.setAttributeValue("Ions", "Ce, Yb");
    cf.setAttributeValue("Symmetries", " D3,  D6h");
    cf.setAttributeValue("PhysicalProperties", "");
    cf.setAttributeValue("Temperatures", std::vector<double>({44}));
    cf.setAttributeValue("FWHMs", std::vector<double>({1}));
    auto source = cf.buildSource();
    std::string Ions = cf.getAttribute("Ions").asString();
    std::string Symmetries = cf.getAttribute("Symmetries").asString();
    std::string PhysicalProperties =
        cf.getAttribute("PhysicalProperties").asString();
    bool isComposite =
        dynamic_cast<Mantid::API::CompositeFunction *>(source.get()) != nullptr;
    bool isMultiSite = cf.isMultiSite();
    bool isMultiSpectrum = cf.isMultiSpectrum();
    int nControls = (int)cf.nFunctions();
    TS_ASSERT_EQUALS(Ions, "\"Ce,Yb\"");
    TS_ASSERT_EQUALS(Symmetries, "\"D3,D6h\"");
    TS_ASSERT_EQUALS(PhysicalProperties, "");
    TS_ASSERT_EQUALS(isComposite, true);
    TS_ASSERT_EQUALS(isMultiSite, true);
    TS_ASSERT_EQUALS(isMultiSpectrum, false);
    TS_ASSERT_EQUALS(nControls, 0);
  }

  void test_build_3() {
    Mantid::CurveFitting::Functions::CrystalFieldControl cf;
    cf.setAttributeValue("Ions", "Ce");
    cf.setAttributeValue("Symmetries", " C2v");
    cf.setAttributeValue("PhysicalProperties", "");
    cf.setAttributeValue("Temperatures", std::vector<double>({44, 50}));
    cf.setAttributeValue("FWHMs", std::vector<double>({1}));
    auto source = cf.buildSource();
    std::string Ions = cf.getAttribute("Ions").asString();
    std::string Symmetries = cf.getAttribute("Symmetries").asString();
    std::string PhysicalProperties =
        cf.getAttribute("PhysicalProperties").asString();
    bool isComposite =
        dynamic_cast<Mantid::API::CompositeFunction *>(source.get()) != nullptr;
    bool isMultiSite = cf.isMultiSite();
    bool isMultiSpectrum = cf.isMultiSpectrum();
    int nControls = (int)cf.nFunctions();
    TS_ASSERT_EQUALS(Ions, "\"Ce\"");
    TS_ASSERT_EQUALS(Symmetries, "\"C2v\"");
    TS_ASSERT_EQUALS(PhysicalProperties, "");
    TS_ASSERT_EQUALS(isComposite, false);
    TS_ASSERT_EQUALS(isMultiSite, false);
    TS_ASSERT_EQUALS(isMultiSpectrum, true);
    TS_ASSERT_EQUALS(nControls, 2);
  }

  void test_build_4() {
    Mantid::CurveFitting::Functions::CrystalFieldControl cf;
    cf.setAttributeValue("Ions", "Ce, Yb");
    cf.setAttributeValue("Symmetries", " D3,  D6h");
    cf.setAttributeValue("PhysicalProperties", "");
    cf.setAttributeValue("Temperatures", std::vector<double>({44, 50}));
    cf.setAttributeValue("FWHMs", std::vector<double>({1}));
    auto source = cf.buildSource();
    std::string Ions = cf.getAttribute("Ions").asString();
    std::string Symmetries = cf.getAttribute("Symmetries").asString();
    std::string PhysicalProperties =
        cf.getAttribute("PhysicalProperties").asString();
    bool isComposite =
        dynamic_cast<Mantid::API::CompositeFunction *>(source.get()) != nullptr;
    bool isMultiSite = cf.isMultiSite();
    bool isMultiSpectrum = cf.isMultiSpectrum();
    int nControls = (int)cf.nFunctions();
    TS_ASSERT_EQUALS(Ions, "\"Ce,Yb\"");
    TS_ASSERT_EQUALS(Symmetries, "\"D3,D6h\"");
    TS_ASSERT_EQUALS(PhysicalProperties, "");
    TS_ASSERT_EQUALS(isComposite, true);
    TS_ASSERT_EQUALS(isMultiSite, true);
    TS_ASSERT_EQUALS(isMultiSpectrum, true);
    TS_ASSERT_EQUALS(nControls, 2);
  }

  void test_build_5() {
    Mantid::CurveFitting::Functions::CrystalFieldControl cf;
    cf.setAttributeValue("Ions", "Ce");
    cf.setAttributeValue("Symmetries", " C2v");
    cf.setAttributeValue("PhysicalProperties", "");
    cf.setAttributeValue("Temperatures", std::vector<double>({44}));
    cf.setAttributeValue("FWHMs", std::vector<double>({1.1, 1.2}));

    // error: Vector of FWHMs must either have same size as Temperatures (1)
    // or have size 1. std::runtime_error
    TS_ASSERT_THROWS(cf.buildSource(), const std::runtime_error &);
  }

  void xtest_build_7() {
    Mantid::CurveFitting::Functions::CrystalFieldControl cf;
    cf.setAttributeValue("Ions", "Ce");
    cf.setAttributeValue("Symmetries", " C2v");
    cf.setAttributeValue("PhysicalProperties", "");
    cf.setAttributeValue("Temperatures", std::vector<double>({44, 50}));
    cf.setAttributeValue("FWHMs", std::vector<double>({1.1, 1.2}));
    cf.buildControls();
    auto source = cf.buildSource();
    std::string Ions = cf.getAttribute("Ions").asString();
    std::string Symmetries = cf.getAttribute("Symmetries").asString();
    std::string PhysicalProperties =
        cf.getAttribute("PhysicalProperties").asString();
    bool isComposite =
        dynamic_cast<Mantid::API::CompositeFunction *>(source.get()) != nullptr;
    bool isMultiSite = cf.isMultiSite();
    bool isMultiSpectrum = cf.isMultiSpectrum();
    int nControls = (int)cf.nFunctions();
    TS_ASSERT_EQUALS(Ions, "\"Ce\"");
    TS_ASSERT_EQUALS(Symmetries, "\"C2v\"");
    TS_ASSERT_EQUALS(PhysicalProperties, "");
    TS_ASSERT_EQUALS(isComposite, false);
    TS_ASSERT_EQUALS(isMultiSite, false);
    TS_ASSERT_EQUALS(isMultiSpectrum, true);
    TS_ASSERT_EQUALS(nControls, 2);
  }

  void test_build_8() {
    Mantid::CurveFitting::Functions::CrystalFieldControl cf;
    cf.setAttributeValue("Ions", "Ce, Yb");
    cf.setAttributeValue("Symmetries", " D3,  D6h");
    cf.setAttributeValue("PhysicalProperties", "");
    cf.setAttributeValue("Temperatures", std::vector<double>({44, 50}));
    cf.setAttributeValue("FWHMs", std::vector<double>({1.1, 1.2}));
    auto source = cf.buildSource();
    std::string Ions = cf.getAttribute("Ions").asString();
    std::string Symmetries = cf.getAttribute("Symmetries").asString();
    std::string PhysicalProperties =
        cf.getAttribute("PhysicalProperties").asString();
    bool isComposite =
        dynamic_cast<Mantid::API::CompositeFunction *>(source.get()) != nullptr;
    bool isMultiSite = cf.isMultiSite();
    bool isMultiSpectrum = cf.isMultiSpectrum();
    int nControls = (int)cf.nFunctions();
    TS_ASSERT_EQUALS(Ions, "\"Ce,Yb\"");
    TS_ASSERT_EQUALS(Symmetries, "\"D3,D6h\"");
    TS_ASSERT_EQUALS(PhysicalProperties, "");
    TS_ASSERT_EQUALS(isComposite, true);
    TS_ASSERT_EQUALS(isMultiSite, true);
    TS_ASSERT_EQUALS(isMultiSpectrum, true);
    TS_ASSERT_EQUALS(nControls, 2);
  }

  void test_build_9() {
    Mantid::CurveFitting::Functions::CrystalFieldControl cf;
    cf.setAttributeValue("Ions", "Ce");
    cf.setAttributeValue("Symmetries", " C2v");
    cf.setAttributeValue("PhysicalProperties", " cv");
    cf.setAttributeValue("Temperatures", std::vector<double>({44}));
    cf.setAttributeValue("FWHMs", std::vector<double>({1}));
    auto source = cf.buildSource();
    std::string Ions = cf.getAttribute("Ions").asString();
    std::string Symmetries = cf.getAttribute("Symmetries").asString();
    std::string PhysicalProperties =
        cf.getAttribute("PhysicalProperties").asString();
    bool isComposite =
        dynamic_cast<Mantid::API::CompositeFunction *>(source.get()) != nullptr;
    bool isMultiSite = cf.isMultiSite();
    bool isMultiSpectrum = cf.isMultiSpectrum();
    int nControls = (int)cf.nFunctions();
    TS_ASSERT_EQUALS(Ions, "\"Ce\"");
    TS_ASSERT_EQUALS(Symmetries, "\"C2v\"");
    TS_ASSERT_EQUALS(PhysicalProperties, "cv");
    TS_ASSERT_EQUALS(isComposite, false);
    TS_ASSERT_EQUALS(isMultiSite, false);
    TS_ASSERT_EQUALS(isMultiSpectrum, true);
    TS_ASSERT_EQUALS(nControls, 0);
  }

  void test_build_10() {
    Mantid::CurveFitting::Functions::CrystalFieldControl cf;
    cf.setAttributeValue("Ions", "Ce, Yb");
    cf.setAttributeValue("Symmetries", " D3,  D6h");
    cf.setAttributeValue("PhysicalProperties", " cv");
    cf.setAttributeValue("Temperatures", std::vector<double>({44}));
    cf.setAttributeValue("FWHMs", std::vector<double>({1}));
    auto source = cf.buildSource();
    std::string Ions = cf.getAttribute("Ions").asString();
    std::string Symmetries = cf.getAttribute("Symmetries").asString();
    std::string PhysicalProperties =
        cf.getAttribute("PhysicalProperties").asString();
    bool isComposite =
        dynamic_cast<Mantid::API::CompositeFunction *>(source.get()) != nullptr;
    bool isMultiSite = cf.isMultiSite();
    bool isMultiSpectrum = cf.isMultiSpectrum();
    int nControls = (int)cf.nFunctions();
    TS_ASSERT_EQUALS(Ions, "\"Ce,Yb\"");
    TS_ASSERT_EQUALS(Symmetries, "\"D3,D6h\"");
    TS_ASSERT_EQUALS(PhysicalProperties, "cv");
    TS_ASSERT_EQUALS(isComposite, true);
    TS_ASSERT_EQUALS(isMultiSite, true);
    TS_ASSERT_EQUALS(isMultiSpectrum, true);
    TS_ASSERT_EQUALS(nControls, 0);
  }

  void test_build_11() {
    Mantid::CurveFitting::Functions::CrystalFieldControl cf;
    cf.setAttributeValue("Ions", "Ce");
    cf.setAttributeValue("Symmetries", " C2v");
    cf.setAttributeValue("PhysicalProperties", " cv");
    cf.setAttributeValue("Temperatures", std::vector<double>({44, 50}));
    cf.setAttributeValue("FWHMs", std::vector<double>({1}));
    auto source = cf.buildSource();
    std::string Ions = cf.getAttribute("Ions").asString();
    std::string Symmetries = cf.getAttribute("Symmetries").asString();
    std::string PhysicalProperties =
        cf.getAttribute("PhysicalProperties").asString();
    bool isComposite =
        dynamic_cast<Mantid::API::CompositeFunction *>(source.get()) != nullptr;
    bool isMultiSite = cf.isMultiSite();
    bool isMultiSpectrum = cf.isMultiSpectrum();
    int nControls = (int)cf.nFunctions();
    TS_ASSERT_EQUALS(Ions, "\"Ce\"");
    TS_ASSERT_EQUALS(Symmetries, "\"C2v\"");
    TS_ASSERT_EQUALS(PhysicalProperties, "cv");
    TS_ASSERT_EQUALS(isComposite, false);
    TS_ASSERT_EQUALS(isMultiSite, false);
    TS_ASSERT_EQUALS(isMultiSpectrum, true);
    TS_ASSERT_EQUALS(nControls, 2);
  }

  void test_build_12() {
    Mantid::CurveFitting::Functions::CrystalFieldControl cf;
    cf.setAttributeValue("Ions", "Ce, Yb");
    cf.setAttributeValue("Symmetries", " D3,  D6h");
    cf.setAttributeValue("PhysicalProperties", " cv");
    cf.setAttributeValue("Temperatures", std::vector<double>({44, 50}));
    cf.setAttributeValue("FWHMs", std::vector<double>({1}));
    auto source = cf.buildSource();
    std::string Ions = cf.getAttribute("Ions").asString();
    std::string Symmetries = cf.getAttribute("Symmetries").asString();
    std::string PhysicalProperties =
        cf.getAttribute("PhysicalProperties").asString();
    bool isComposite =
        dynamic_cast<Mantid::API::CompositeFunction *>(source.get()) != nullptr;
    bool isMultiSite = cf.isMultiSite();
    bool isMultiSpectrum = cf.isMultiSpectrum();
    int nControls = (int)cf.nFunctions();
    TS_ASSERT_EQUALS(Ions, "\"Ce,Yb\"");
    TS_ASSERT_EQUALS(Symmetries, "\"D3,D6h\"");
    TS_ASSERT_EQUALS(PhysicalProperties, "cv");
    TS_ASSERT_EQUALS(isComposite, true);
    TS_ASSERT_EQUALS(isMultiSite, true);
    TS_ASSERT_EQUALS(isMultiSpectrum, true);
    TS_ASSERT_EQUALS(nControls, 2);
  }

  void test_build_15() {
    Mantid::CurveFitting::Functions::CrystalFieldControl cf;
    cf.setAttributeValue("Ions", "Ce");
    cf.setAttributeValue("Symmetries", " C2v");
    cf.setAttributeValue("PhysicalProperties", " cv");
    cf.setAttributeValue("Temperatures", std::vector<double>({44, 50}));
    cf.setAttributeValue("FWHMs", std::vector<double>({1.1, 1.2}));
    auto source = cf.buildSource();
    std::string Ions = cf.getAttribute("Ions").asString();
    std::string Symmetries = cf.getAttribute("Symmetries").asString();
    std::string PhysicalProperties =
        cf.getAttribute("PhysicalProperties").asString();
    bool isComposite =
        dynamic_cast<Mantid::API::CompositeFunction *>(source.get()) != nullptr;
    bool isMultiSite = cf.isMultiSite();
    bool isMultiSpectrum = cf.isMultiSpectrum();
    int nControls = (int)cf.nFunctions();
    TS_ASSERT_EQUALS(Ions, "\"Ce\"");
    TS_ASSERT_EQUALS(Symmetries, "\"C2v\"");
    TS_ASSERT_EQUALS(PhysicalProperties, "cv");
    TS_ASSERT_EQUALS(isComposite, false);
    TS_ASSERT_EQUALS(isMultiSite, false);
    TS_ASSERT_EQUALS(isMultiSpectrum, true);
    TS_ASSERT_EQUALS(nControls, 2);
  }

  void test_build_16() {
    Mantid::CurveFitting::Functions::CrystalFieldControl cf;
    cf.setAttributeValue("Ions", "Ce, Yb");
    cf.setAttributeValue("Symmetries", " D3,  D6h");
    cf.setAttributeValue("PhysicalProperties", " cv");
    cf.setAttributeValue("Temperatures", std::vector<double>({44, 50}));
    cf.setAttributeValue("FWHMs", std::vector<double>({1.1, 1.2}));
    auto source = cf.buildSource();
    std::string Ions = cf.getAttribute("Ions").asString();
    std::string Symmetries = cf.getAttribute("Symmetries").asString();
    std::string PhysicalProperties =
        cf.getAttribute("PhysicalProperties").asString();
    bool isComposite =
        dynamic_cast<Mantid::API::CompositeFunction *>(source.get()) != nullptr;
    bool isMultiSite = cf.isMultiSite();
    bool isMultiSpectrum = cf.isMultiSpectrum();
    int nControls = (int)cf.nFunctions();
    TS_ASSERT_EQUALS(Ions, "\"Ce,Yb\"");
    TS_ASSERT_EQUALS(Symmetries, "\"D3,D6h\"");
    TS_ASSERT_EQUALS(PhysicalProperties, "cv");
    TS_ASSERT_EQUALS(isComposite, true);
    TS_ASSERT_EQUALS(isMultiSite, true);
    TS_ASSERT_EQUALS(isMultiSpectrum, true);
    TS_ASSERT_EQUALS(nControls, 2);
  }
};

#endif /*CRYSTALFIELDCONTROLTEST_H_*/
