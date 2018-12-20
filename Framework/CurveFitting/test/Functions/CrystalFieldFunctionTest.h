#ifndef CRYSTALFIELDFUNCTIONTEST_H_
#define CRYSTALFIELDFUNCTIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AlgorithmFactory.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidCurveFitting/Algorithms/EvaluateFunction.h"
#include "MantidCurveFitting/Algorithms/Fit.h"
#include "MantidCurveFitting/Functions/CrystalFieldFunction.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::CurveFitting;
using namespace Mantid::CurveFitting::Functions;
using namespace WorkspaceCreationHelper;

class CrystalFieldFunctionTest : public CxxTest::TestSuite {
public:
  void test_names_ss() {
    CrystalFieldFunction cf;
    TS_ASSERT_EQUALS(cf.nParams(), 0);

    cf.setAttributeValue("Ions", "Ce");
    cf.setAttributeValue("Symmetries", "C2v");
    cf.setAttributeValue("Temperatures", std::vector<double>({44}));
    cf.setAttributeValue("FWHMs", std::vector<double>({1}));

    auto attributeNames = cf.getAttributeNames();
    TS_ASSERT_EQUALS(attributeNames.size(), 15);
    TS_ASSERT_EQUALS(cf.nAttributes(), 15);
    TS_ASSERT_EQUALS(attributeNames[0], "Ions");
    TS_ASSERT_EQUALS(attributeNames[1], "Symmetries");
    TS_ASSERT_EQUALS(attributeNames[2], "Temperatures");

    TS_ASSERT_EQUALS(cf.nParams(), 40);
    TS_ASSERT_EQUALS(cf.parameterName(0), "BmolX");
    TS_ASSERT_EQUALS(cf.parameterName(6), "B20");
    TS_ASSERT_EQUALS(cf.parameterName(26), "IB44");
    TS_ASSERT_EQUALS(cf.parameterName(33), "IntensityScaling");
    TS_ASSERT_EQUALS(cf.parameterName(34), "pk0.Amplitude");
    TS_ASSERT_EQUALS(cf.parameterName(38), "pk1.PeakCentre");

    TS_ASSERT_EQUALS(cf.parameterIndex("BmolX"), 0);
    TS_ASSERT_EQUALS(cf.parameterIndex("B20"), 6);
    TS_ASSERT_EQUALS(cf.parameterIndex("IB44"), 26);
    TS_ASSERT_EQUALS(cf.parameterIndex("IntensityScaling"), 33);
    TS_ASSERT_EQUALS(cf.parameterIndex("pk0.Amplitude"), 34);
    TS_ASSERT_EQUALS(cf.parameterIndex("pk1.PeakCentre"), 38);

    auto fun = FunctionFactory::Instance().createInitialized(cf.asString());
    TS_ASSERT_EQUALS(fun->getAttribute("Ions").asUnquotedString(), "Ce");
    TS_ASSERT_EQUALS(fun->getAttribute("Symmetries").asUnquotedString(), "C2v");
    TS_ASSERT_EQUALS(fun->getAttribute("Temperatures").asVector(),
                     std::vector<double>({44}));
    TS_ASSERT_EQUALS(fun->getAttribute("FWHMs").asVector(),
                     std::vector<double>({1}));
  }

  void test_names_ss_with_backgrund() {
    CrystalFieldFunction cf;
    cf.setAttributeValue("Ions", "Ce");
    cf.setAttributeValue("Symmetries", "C2v");
    cf.setAttributeValue("Temperatures", std::vector<double>({44}));
    cf.setAttributeValue("FWHMs", std::vector<double>({1}));
    cf.setAttributeValue("Background", "name=FlatBackground");
    auto attributeNames = cf.getAttributeNames();
    TS_ASSERT_EQUALS(attributeNames.size(), 15);
    TS_ASSERT_EQUALS(cf.nAttributes(), 15);
    TS_ASSERT_EQUALS(attributeNames[0], "Ions");
    TS_ASSERT_EQUALS(attributeNames[1], "Symmetries");
    TS_ASSERT_EQUALS(attributeNames[2], "Temperatures");

    TS_ASSERT_EQUALS(cf.parameterName(0), "BmolX");
    TS_ASSERT_EQUALS(cf.parameterName(6), "B20");
    TS_ASSERT_EQUALS(cf.parameterName(26), "IB44");
    TS_ASSERT_EQUALS(cf.parameterName(33), "IntensityScaling");

    TS_ASSERT_EQUALS(cf.parameterName(34), "bg.A0");
    TS_ASSERT_EQUALS(cf.parameterName(35), "pk0.Amplitude");
    TS_ASSERT_EQUALS(cf.parameterName(39), "pk1.PeakCentre");

    TS_ASSERT_EQUALS(cf.parameterIndex("BmolX"), 0);
    TS_ASSERT_EQUALS(cf.parameterIndex("B20"), 6);
    TS_ASSERT_EQUALS(cf.parameterIndex("IB44"), 26);
    TS_ASSERT_EQUALS(cf.parameterIndex("IntensityScaling"), 33);
    TS_ASSERT_EQUALS(cf.parameterIndex("bg.A0"), 34);
    TS_ASSERT_EQUALS(cf.parameterIndex("pk0.Amplitude"), 35);
    TS_ASSERT_EQUALS(cf.parameterIndex("pk1.PeakCentre"), 39);

    auto fun = FunctionFactory::Instance().createInitialized(cf.asString());
    TS_ASSERT_EQUALS(fun->getAttribute("Ions").asUnquotedString(), "Ce");
    TS_ASSERT_EQUALS(fun->getAttribute("Symmetries").asUnquotedString(), "C2v");
    TS_ASSERT_EQUALS(fun->getAttribute("Temperatures").asVector(),
                     std::vector<double>({44}));
    TS_ASSERT_EQUALS(fun->getAttribute("FWHMs").asVector(),
                     std::vector<double>({1}));
  }

  void test_names_sm() {
    CrystalFieldFunction cf;
    cf.setAttributeValue("Ions", "Ce");
    cf.setAttributeValue("Symmetries", "C2v");
    cf.setAttributeValue("Temperatures", std::vector<double>({44, 50}));
    cf.setAttributeValue("FWHMs", std::vector<double>({1, 2}));

    auto attributeNames = cf.getAttributeNames();
    TS_ASSERT_EQUALS(attributeNames.size(), 17);
    TS_ASSERT_EQUALS(cf.nAttributes(), 17);
    TS_ASSERT_EQUALS(attributeNames[0], "Ions");
    TS_ASSERT_EQUALS(attributeNames[1], "Symmetries");
    TS_ASSERT_EQUALS(attributeNames[2], "Temperatures");
    TS_ASSERT_EQUALS(attributeNames[13], "sp0.FWHMX");
    TS_ASSERT_EQUALS(attributeNames[16], "sp1.FWHMY");

    TS_ASSERT_EQUALS(cf.parameterName(0), "sp0.IntensityScaling");
    TS_ASSERT_EQUALS(cf.parameterName(1), "sp1.IntensityScaling");

    TS_ASSERT_EQUALS(cf.parameterName(2), "BmolX");
    TS_ASSERT_EQUALS(cf.parameterName(8), "B20");
    TS_ASSERT_EQUALS(cf.parameterName(28), "IB44");
    TS_ASSERT_EQUALS(cf.parameterName(35), "sp0.pk0.Amplitude");
    TS_ASSERT_EQUALS(cf.parameterName(39), "sp0.pk1.PeakCentre");
    TS_ASSERT_EQUALS(cf.parameterName(41), "sp1.pk0.Amplitude");
    TS_ASSERT_EQUALS(cf.parameterName(45), "sp1.pk1.PeakCentre");

    TS_ASSERT_EQUALS(cf.parameterIndex("sp0.IntensityScaling"), 0);
    TS_ASSERT_EQUALS(cf.parameterIndex("sp1.IntensityScaling"), 1);
    TS_ASSERT_EQUALS(cf.parameterIndex("BmolX"), 2);
    TS_ASSERT_EQUALS(cf.parameterIndex("B20"), 8);
    TS_ASSERT_EQUALS(cf.parameterIndex("IB44"), 28);
    TS_ASSERT_EQUALS(cf.parameterIndex("sp0.pk0.Amplitude"), 35);
    TS_ASSERT_EQUALS(cf.parameterIndex("sp0.pk1.PeakCentre"), 39);
    TS_ASSERT_EQUALS(cf.parameterIndex("sp1.pk0.Amplitude"), 41);
    TS_ASSERT_EQUALS(cf.parameterIndex("sp1.pk1.PeakCentre"), 45);

    auto fun = FunctionFactory::Instance().createInitialized(cf.asString());
    TS_ASSERT_EQUALS(fun->getAttribute("Ions").asUnquotedString(), "Ce");
    TS_ASSERT_EQUALS(fun->getAttribute("Symmetries").asUnquotedString(), "C2v");
    TS_ASSERT_EQUALS(fun->getAttribute("Temperatures").asVector(),
                     std::vector<double>({44, 50}));
    TS_ASSERT_EQUALS(fun->getAttribute("FWHMs").asVector(),
                     std::vector<double>({1, 2}));
  }

  void test_names_sm_with_background() {
    CrystalFieldFunction cf;
    cf.setAttributeValue("Ions", "Ce");
    cf.setAttributeValue("Symmetries", "C2v");
    cf.setAttributeValue("Temperatures", std::vector<double>({44, 50}));
    cf.setAttributeValue("FWHMs", std::vector<double>({1, 2}));
    cf.setAttributeValue("Background", "name=LinearBackground");

    auto attributeNames = cf.getAttributeNames();
    TS_ASSERT_EQUALS(attributeNames.size(), 17);
    TS_ASSERT_EQUALS(cf.nAttributes(), 17);
    TS_ASSERT_EQUALS(attributeNames[0], "Ions");
    TS_ASSERT_EQUALS(attributeNames[1], "Symmetries");
    TS_ASSERT_EQUALS(attributeNames[2], "Temperatures");
    TS_ASSERT_EQUALS(attributeNames[13], "sp0.FWHMX");
    TS_ASSERT_EQUALS(attributeNames[16], "sp1.FWHMY");

    TS_ASSERT_EQUALS(cf.parameterName(0), "sp0.IntensityScaling");
    TS_ASSERT_EQUALS(cf.parameterName(1), "sp1.IntensityScaling");

    TS_ASSERT_EQUALS(cf.parameterName(2), "BmolX");
    TS_ASSERT_EQUALS(cf.parameterName(8), "B20");
    TS_ASSERT_EQUALS(cf.parameterName(28), "IB44");
    TS_ASSERT_EQUALS(cf.parameterName(35), "sp0.bg.A0");
    TS_ASSERT_EQUALS(cf.parameterName(37), "sp0.pk0.Amplitude");
    TS_ASSERT_EQUALS(cf.parameterName(41), "sp0.pk1.PeakCentre");
    TS_ASSERT_EQUALS(cf.parameterName(44), "sp1.bg.A1");
    TS_ASSERT_EQUALS(cf.parameterName(45), "sp1.pk0.Amplitude");
    TS_ASSERT_EQUALS(cf.parameterName(49), "sp1.pk1.PeakCentre");

    TS_ASSERT_EQUALS(cf.parameterIndex("sp0.IntensityScaling"), 0);
    TS_ASSERT_EQUALS(cf.parameterIndex("sp1.IntensityScaling"), 1);
    TS_ASSERT_EQUALS(cf.parameterIndex("BmolX"), 2);
    TS_ASSERT_EQUALS(cf.parameterIndex("B20"), 8);
    TS_ASSERT_EQUALS(cf.parameterIndex("IB44"), 28);
    TS_ASSERT_EQUALS(cf.parameterIndex("sp0.bg.A0"), 35);
    TS_ASSERT_EQUALS(cf.parameterIndex("sp0.pk0.Amplitude"), 37);
    TS_ASSERT_EQUALS(cf.parameterIndex("sp0.pk1.PeakCentre"), 41);
    TS_ASSERT_EQUALS(cf.parameterIndex("sp1.bg.A1"), 44);
    TS_ASSERT_EQUALS(cf.parameterIndex("sp1.pk0.Amplitude"), 45);
    TS_ASSERT_EQUALS(cf.parameterIndex("sp1.pk1.PeakCentre"), 49);

    auto fun = FunctionFactory::Instance().createInitialized(cf.asString());
    TS_ASSERT_EQUALS(fun->getAttribute("Ions").asUnquotedString(), "Ce");
    TS_ASSERT_EQUALS(fun->getAttribute("Symmetries").asUnquotedString(), "C2v");
    TS_ASSERT_EQUALS(fun->getAttribute("Temperatures").asVector(),
                     std::vector<double>({44, 50}));
    TS_ASSERT_EQUALS(fun->getAttribute("FWHMs").asVector(),
                     std::vector<double>({1, 2}));
  }

  void test_names_ms() {
    CrystalFieldFunction cf;
    cf.setAttributeValue("Ions", "Ce, Yb");
    cf.setAttributeValue("Symmetries", "C2v, D6h");
    cf.setAttributeValue("Temperatures", std::vector<double>({44}));
    cf.setAttributeValue("FWHMs", std::vector<double>({1}));
    auto attributeNames = cf.getAttributeNames();
    TS_ASSERT_EQUALS(attributeNames.size(), 15);
    TS_ASSERT_EQUALS(cf.nAttributes(), 15);
    TS_ASSERT_EQUALS(attributeNames[0], "Ions");
    TS_ASSERT_EQUALS(attributeNames[1], "Symmetries");
    TS_ASSERT_EQUALS(attributeNames[2], "Temperatures");

    TS_ASSERT_EQUALS(cf.parameterName(0), "ion0.BmolX");
    TS_ASSERT_EQUALS(cf.parameterName(6), "ion0.B20");
    TS_ASSERT_EQUALS(cf.parameterName(26), "ion0.IB44");
    TS_ASSERT_EQUALS(cf.parameterName(33), "ion0.IntensityScaling");

    TS_ASSERT_EQUALS(cf.parameterName(34), "ion1.BmolX");
    TS_ASSERT_EQUALS(cf.parameterName(40), "ion1.B20");
    TS_ASSERT_EQUALS(cf.parameterName(60), "ion1.IB44");
    TS_ASSERT_EQUALS(cf.parameterName(67), "ion1.IntensityScaling");

    TS_ASSERT_EQUALS(cf.parameterName(68), "ion0.pk0.Amplitude");
    TS_ASSERT_EQUALS(cf.parameterName(72), "ion0.pk1.PeakCentre");

    TS_ASSERT_EQUALS(cf.parameterName(74), "ion1.pk0.Amplitude");
    TS_ASSERT_EQUALS(cf.parameterName(79), "ion1.pk1.FWHM");

    TS_ASSERT_EQUALS(cf.parameterIndex("ion0.BmolX"), 0);
    TS_ASSERT_EQUALS(cf.parameterIndex("ion0.B20"), 6);
    TS_ASSERT_EQUALS(cf.parameterIndex("ion0.IB44"), 26);
    TS_ASSERT_EQUALS(cf.parameterIndex("ion0.IntensityScaling"), 33);

    TS_ASSERT_EQUALS(cf.parameterIndex("ion1.BmolX"), 34);
    TS_ASSERT_EQUALS(cf.parameterIndex("ion1.B20"), 40);
    TS_ASSERT_EQUALS(cf.parameterIndex("ion1.IB44"), 60);
    TS_ASSERT_EQUALS(cf.parameterIndex("ion1.IntensityScaling"), 67);

    TS_ASSERT_EQUALS(cf.parameterIndex("ion0.pk0.Amplitude"), 68);
    TS_ASSERT_EQUALS(cf.parameterIndex("ion0.pk1.PeakCentre"), 72);

    TS_ASSERT_EQUALS(cf.parameterIndex("ion1.pk0.Amplitude"), 74);
    TS_ASSERT_EQUALS(cf.parameterIndex("ion1.pk1.FWHM"), 79);

    auto fun = FunctionFactory::Instance().createInitialized(cf.asString());
    TS_ASSERT_EQUALS(fun->getAttribute("Ions").asUnquotedString(), "Ce,Yb");
    TS_ASSERT_EQUALS(fun->getAttribute("Symmetries").asUnquotedString(),
                     "C2v,D6h");
    TS_ASSERT_EQUALS(fun->getAttribute("Temperatures").asVector(),
                     std::vector<double>({44}));
    TS_ASSERT_EQUALS(fun->getAttribute("FWHMs").asVector(),
                     std::vector<double>({1}));
  }

  void test_names_ms_with_background() {
    CrystalFieldFunction cf;
    cf.setAttributeValue("Ions", "Ce, Yb");
    cf.setAttributeValue("Symmetries", "C2v, D6h");
    cf.setAttributeValue("Temperatures", std::vector<double>({44}));
    cf.setAttributeValue("FWHMs", std::vector<double>({1}));
    cf.setAttributeValue("Background", "name=LinearBackground");
    auto attributeNames = cf.getAttributeNames();
    TS_ASSERT_EQUALS(attributeNames.size(), 15);
    TS_ASSERT_EQUALS(cf.nAttributes(), 15);
    TS_ASSERT_EQUALS(attributeNames[0], "Ions");
    TS_ASSERT_EQUALS(attributeNames[1], "Symmetries");
    TS_ASSERT_EQUALS(attributeNames[2], "Temperatures");

    TS_ASSERT_EQUALS(cf.parameterName(0), "ion0.BmolX");
    TS_ASSERT_EQUALS(cf.parameterName(6), "ion0.B20");
    TS_ASSERT_EQUALS(cf.parameterName(26), "ion0.IB44");
    TS_ASSERT_EQUALS(cf.parameterName(33), "ion0.IntensityScaling");

    TS_ASSERT_EQUALS(cf.parameterName(34), "ion1.BmolX");
    TS_ASSERT_EQUALS(cf.parameterName(40), "ion1.B20");
    TS_ASSERT_EQUALS(cf.parameterName(60), "ion1.IB44");
    TS_ASSERT_EQUALS(cf.parameterName(67), "ion1.IntensityScaling");

    TS_ASSERT_EQUALS(cf.parameterName(68), "bg.A0");
    TS_ASSERT_EQUALS(cf.parameterName(69), "bg.A1");
    TS_ASSERT_EQUALS(cf.parameterName(70), "ion0.pk0.Amplitude");
    TS_ASSERT_EQUALS(cf.parameterName(74), "ion0.pk1.PeakCentre");
    TS_ASSERT_EQUALS(cf.parameterName(76), "ion1.pk0.Amplitude");
    TS_ASSERT_EQUALS(cf.parameterName(81), "ion1.pk1.FWHM");

    TS_ASSERT_EQUALS(cf.parameterIndex("ion0.BmolX"), 0);
    TS_ASSERT_EQUALS(cf.parameterIndex("ion0.B20"), 6);
    TS_ASSERT_EQUALS(cf.parameterIndex("ion0.IB44"), 26);
    TS_ASSERT_EQUALS(cf.parameterIndex("ion0.IntensityScaling"), 33);

    TS_ASSERT_EQUALS(cf.parameterIndex("ion1.BmolX"), 34);
    TS_ASSERT_EQUALS(cf.parameterIndex("ion1.B20"), 40);
    TS_ASSERT_EQUALS(cf.parameterIndex("ion1.IB44"), 60);
    TS_ASSERT_EQUALS(cf.parameterIndex("ion1.IntensityScaling"), 67);

    TS_ASSERT_EQUALS(cf.parameterIndex("bg.A0"), 68);
    TS_ASSERT_EQUALS(cf.parameterIndex("bg.A1"), 69);
    TS_ASSERT_EQUALS(cf.parameterIndex("ion0.pk0.Amplitude"), 70);
    TS_ASSERT_EQUALS(cf.parameterIndex("ion0.pk1.PeakCentre"), 74);
    TS_ASSERT_EQUALS(cf.parameterIndex("ion1.pk0.Amplitude"), 76);
    TS_ASSERT_EQUALS(cf.parameterIndex("ion1.pk1.FWHM"), 81);

    auto fun = FunctionFactory::Instance().createInitialized(cf.asString());
    TS_ASSERT_EQUALS(fun->getAttribute("Ions").asUnquotedString(), "Ce,Yb");
    TS_ASSERT_EQUALS(fun->getAttribute("Symmetries").asUnquotedString(),
                     "C2v,D6h");
    TS_ASSERT_EQUALS(fun->getAttribute("Temperatures").asVector(),
                     std::vector<double>({44}));
    TS_ASSERT_EQUALS(fun->getAttribute("FWHMs").asVector(),
                     std::vector<double>({1}));
  }

  void test_names_mm() {
    CrystalFieldFunction cf;
    cf.setAttributeValue("Ions", "Ce, Yb");
    cf.setAttributeValue("Symmetries", "C2v, D6h");
    cf.setAttributeValue("Temperatures", std::vector<double>({44, 50}));
    cf.setAttributeValue("FWHMs", std::vector<double>({1, 2}));
    auto attributeNames = cf.getAttributeNames();
    TS_ASSERT_EQUALS(attributeNames.size(), 17);
    TS_ASSERT_EQUALS(cf.nAttributes(), 17);
    TS_ASSERT_EQUALS(attributeNames[0], "Ions");
    TS_ASSERT_EQUALS(attributeNames[1], "Symmetries");
    TS_ASSERT_EQUALS(attributeNames[2], "Temperatures");

    TS_ASSERT_EQUALS(cf.parameterName(0), "sp0.IntensityScaling");
    TS_ASSERT_EQUALS(cf.parameterName(1), "sp1.IntensityScaling");
    TS_ASSERT_EQUALS(cf.parameterName(2), "ion0.BmolX");
    TS_ASSERT_EQUALS(cf.parameterName(8), "ion0.B20");
    TS_ASSERT_EQUALS(cf.parameterName(28), "ion0.IB44");
    TS_ASSERT_EQUALS(cf.parameterName(35), "ion0.IntensityScaling");

    TS_ASSERT_EQUALS(cf.parameterName(36), "ion1.BmolX");
    TS_ASSERT_EQUALS(cf.parameterName(42), "ion1.B20");
    TS_ASSERT_EQUALS(cf.parameterName(62), "ion1.IB44");
    TS_ASSERT_EQUALS(cf.parameterName(69), "ion1.IntensityScaling");

    TS_ASSERT_EQUALS(cf.parameterName(70), "ion0.sp0.pk0.Amplitude");
    TS_ASSERT_EQUALS(cf.parameterName(74), "ion0.sp0.pk1.PeakCentre");
    TS_ASSERT_EQUALS(cf.parameterName(76), "ion1.sp0.pk0.Amplitude");
    TS_ASSERT_EQUALS(cf.parameterName(81), "ion1.sp0.pk1.FWHM");
    TS_ASSERT_EQUALS(cf.parameterName(82), "ion0.sp1.pk0.Amplitude");
    TS_ASSERT_EQUALS(cf.parameterName(86), "ion0.sp1.pk1.PeakCentre");
    TS_ASSERT_EQUALS(cf.parameterName(88), "ion1.sp1.pk0.Amplitude");
    TS_ASSERT_EQUALS(cf.parameterName(93), "ion1.sp1.pk1.FWHM");

    TS_ASSERT_EQUALS(cf.parameterIndex("sp0.IntensityScaling"), 0);
    TS_ASSERT_EQUALS(cf.parameterIndex("sp1.IntensityScaling"), 1);
    TS_ASSERT_EQUALS(cf.parameterIndex("ion0.BmolX"), 2);
    TS_ASSERT_EQUALS(cf.parameterIndex("ion0.B20"), 8);
    TS_ASSERT_EQUALS(cf.parameterIndex("ion0.IB44"), 28);
    TS_ASSERT_EQUALS(cf.parameterIndex("ion0.IntensityScaling"), 35);

    TS_ASSERT_EQUALS(cf.parameterIndex("ion1.BmolX"), 36);
    TS_ASSERT_EQUALS(cf.parameterIndex("ion1.B20"), 42);
    TS_ASSERT_EQUALS(cf.parameterIndex("ion1.IB44"), 62);
    TS_ASSERT_EQUALS(cf.parameterIndex("ion1.IntensityScaling"), 69);

    TS_ASSERT_EQUALS(cf.parameterIndex("ion0.sp0.pk0.Amplitude"), 70);
    TS_ASSERT_EQUALS(cf.parameterIndex("ion0.sp0.pk1.PeakCentre"), 74);
    TS_ASSERT_EQUALS(cf.parameterIndex("ion1.sp0.pk0.Amplitude"), 76);
    TS_ASSERT_EQUALS(cf.parameterIndex("ion1.sp0.pk1.FWHM"), 81);
    TS_ASSERT_EQUALS(cf.parameterIndex("ion0.sp1.pk0.Amplitude"), 82);
    TS_ASSERT_EQUALS(cf.parameterIndex("ion0.sp1.pk1.PeakCentre"), 86);
    TS_ASSERT_EQUALS(cf.parameterIndex("ion1.sp1.pk0.Amplitude"), 88);
    TS_ASSERT_EQUALS(cf.parameterIndex("ion1.sp1.pk1.FWHM"), 93);

    auto fun = FunctionFactory::Instance().createInitialized(cf.asString());
    TS_ASSERT_EQUALS(fun->getAttribute("Ions").asUnquotedString(), "Ce,Yb");
    TS_ASSERT_EQUALS(fun->getAttribute("Symmetries").asUnquotedString(),
                     "C2v,D6h");
    TS_ASSERT_EQUALS(fun->getAttribute("Temperatures").asVector(),
                     std::vector<double>({44, 50}));
    TS_ASSERT_EQUALS(fun->getAttribute("FWHMs").asVector(),
                     std::vector<double>({1, 2}));
  }

  void test_names_mm_with_background() {
    CrystalFieldFunction cf;
    cf.setAttributeValue("Ions", "Ce, Yb");
    cf.setAttributeValue("Symmetries", "C2v, D6h");
    cf.setAttributeValue("Temperatures", std::vector<double>({44, 50}));
    cf.setAttributeValue("FWHMs", std::vector<double>({1, 2}));
    cf.setAttributeValue("Background", "name=LinearBackground");
    auto attributeNames = cf.getAttributeNames();
    TS_ASSERT_EQUALS(attributeNames.size(), 17);
    TS_ASSERT_EQUALS(cf.nAttributes(), 17);
    TS_ASSERT_EQUALS(attributeNames[0], "Ions");
    TS_ASSERT_EQUALS(attributeNames[1], "Symmetries");
    TS_ASSERT_EQUALS(attributeNames[2], "Temperatures");
    TS_ASSERT_EQUALS(cf.parameterName(0), "sp0.IntensityScaling");
    TS_ASSERT_EQUALS(cf.parameterName(1), "sp1.IntensityScaling");
    TS_ASSERT_EQUALS(cf.parameterName(2), "ion0.BmolX");
    TS_ASSERT_EQUALS(cf.parameterName(8), "ion0.B20");
    TS_ASSERT_EQUALS(cf.parameterName(28), "ion0.IB44");
    TS_ASSERT_EQUALS(cf.parameterName(35), "ion0.IntensityScaling");

    TS_ASSERT_EQUALS(cf.parameterName(36), "ion1.BmolX");
    TS_ASSERT_EQUALS(cf.parameterName(42), "ion1.B20");
    TS_ASSERT_EQUALS(cf.parameterName(62), "ion1.IB44");
    TS_ASSERT_EQUALS(cf.parameterName(69), "ion1.IntensityScaling");

    TS_ASSERT_EQUALS(cf.parameterName(70), "sp0.bg.A0");
    TS_ASSERT_EQUALS(cf.parameterName(71), "sp0.bg.A1");
    TS_ASSERT_EQUALS(cf.parameterName(72), "ion0.sp0.pk0.Amplitude");
    TS_ASSERT_EQUALS(cf.parameterName(76), "ion0.sp0.pk1.PeakCentre");
    TS_ASSERT_EQUALS(cf.parameterName(78), "ion1.sp0.pk0.Amplitude");
    TS_ASSERT_EQUALS(cf.parameterName(83), "ion1.sp0.pk1.FWHM");
    TS_ASSERT_EQUALS(cf.parameterName(84), "sp1.bg.A0");
    TS_ASSERT_EQUALS(cf.parameterName(85), "sp1.bg.A1");
    TS_ASSERT_EQUALS(cf.parameterName(86), "ion0.sp1.pk0.Amplitude");
    TS_ASSERT_EQUALS(cf.parameterName(90), "ion0.sp1.pk1.PeakCentre");
    TS_ASSERT_EQUALS(cf.parameterName(92), "ion1.sp1.pk0.Amplitude");
    TS_ASSERT_EQUALS(cf.parameterName(97), "ion1.sp1.pk1.FWHM");

    TS_ASSERT_EQUALS(cf.parameterIndex("sp0.IntensityScaling"), 0);
    TS_ASSERT_EQUALS(cf.parameterIndex("sp1.IntensityScaling"), 1);
    TS_ASSERT_EQUALS(cf.parameterIndex("ion0.BmolX"), 2);
    TS_ASSERT_EQUALS(cf.parameterIndex("ion0.B20"), 8);
    TS_ASSERT_EQUALS(cf.parameterIndex("ion0.IB44"), 28);
    TS_ASSERT_EQUALS(cf.parameterIndex("ion0.IntensityScaling"), 35);

    TS_ASSERT_EQUALS(cf.parameterIndex("ion1.BmolX"), 36);
    TS_ASSERT_EQUALS(cf.parameterIndex("ion1.B20"), 42);
    TS_ASSERT_EQUALS(cf.parameterIndex("ion1.IB44"), 62);
    TS_ASSERT_EQUALS(cf.parameterIndex("ion1.IntensityScaling"), 69);

    TS_ASSERT_EQUALS(cf.parameterIndex("sp0.bg.A0"), 70);
    TS_ASSERT_EQUALS(cf.parameterIndex("sp0.bg.A1"), 71);
    TS_ASSERT_EQUALS(cf.parameterIndex("ion0.sp0.pk0.Amplitude"), 72);
    TS_ASSERT_EQUALS(cf.parameterIndex("ion0.sp0.pk1.PeakCentre"), 76);
    TS_ASSERT_EQUALS(cf.parameterIndex("ion1.sp0.pk0.Amplitude"), 78);
    TS_ASSERT_EQUALS(cf.parameterIndex("ion1.sp0.pk1.FWHM"), 83);
    TS_ASSERT_EQUALS(cf.parameterIndex("sp1.bg.A0"), 84);
    TS_ASSERT_EQUALS(cf.parameterIndex("sp1.bg.A1"), 85);
    TS_ASSERT_EQUALS(cf.parameterIndex("ion0.sp1.pk0.Amplitude"), 86);
    TS_ASSERT_EQUALS(cf.parameterIndex("ion0.sp1.pk1.PeakCentre"), 90);
    TS_ASSERT_EQUALS(cf.parameterIndex("ion1.sp1.pk0.Amplitude"), 92);
    TS_ASSERT_EQUALS(cf.parameterIndex("ion1.sp1.pk1.FWHM"), 97);

    auto fun = FunctionFactory::Instance().createInitialized(cf.asString());
    TS_ASSERT_EQUALS(fun->getAttribute("Ions").asUnquotedString(), "Ce,Yb");
    TS_ASSERT_EQUALS(fun->getAttribute("Symmetries").asUnquotedString(),
                     "C2v,D6h");
    TS_ASSERT_EQUALS(fun->getAttribute("Temperatures").asVector(),
                     std::vector<double>({44, 50}));
    TS_ASSERT_EQUALS(fun->getAttribute("FWHMs").asVector(),
                     std::vector<double>({1, 2}));
  }

  void test_fit_ss() {
    std::string fun =
        "name=CrystalFieldFunction,Ions=Ce,Symmetries=C2v,"
        "Temperatures=44,FWHMs=2.3,ToleranceIntensity=0.2,B20="
        "0.37,B22=3.9,B40=-0.03,B42=-0.1,B44=-0.12,pk0.FWHM=2.2,"
        "pk1.FWHM=1.8,ties=(B60=0,B62=0,B64=0,B66=0,BmolX=0,"
        "BmolY=0,BmolZ=0,BextX=0,BextY=0,BextZ=0, IntensityScaling=1)";
    auto ws = makeDataSS();
    Algorithms::Fit fit;
    fit.initialize();
    fit.setProperty("Function", fun);
    fit.setProperty("InputWorkspace", ws);
    fit.setProperty("WorkspaceIndex", 1);
    fit.setProperty("Minimizer", "Levenberg-Marquardt");
    fit.setProperty("CalcErrors", true);
    fit.setProperty("Output", "fit_ss");
    fit.execute();

    double chi2 = fit.getProperty("OutputChi2overDoF");
    TS_ASSERT_DELTA(chi2, 0.0, 1e-6);

    API::AnalysisDataService::Instance().clear();
  }

  void test_fit_sm() {
    auto ws = makeDataSM();
    auto sp0 = boost::dynamic_pointer_cast<MatrixWorkspace>(ws->getItem(0));
    auto sp1 = boost::dynamic_pointer_cast<MatrixWorkspace>(ws->getItem(1));

    std::string fun =
        "name=CrystalFieldFunction,Ions=Ce,Symmetries=C2v,"
        "Temperatures=(10, 50),FWHMs=2.1,ToleranceIntensity=0.1,"
        "B20=0.37737,B22=3.9770,B40=-0.031787,B42=-0.11611,B44=-0.12544,"
        "ties=(B60=0,B62=0,B64=0,B66=0,BmolX=0,"
        "BmolY=0,BmolZ=0,BextX=0,BextY=0,BextZ=0, sp0.IntensityScaling=1, "
        "sp1.IntensityScaling=1)";

    Algorithms::Fit fit;
    fit.initialize();
    fit.setProperty("Function", fun);
    fit.setProperty("InputWorkspace", sp0);
    fit.setProperty("WorkspaceIndex", 1);
    fit.setProperty("InputWorkspace_1", sp1);
    fit.setProperty("WorkspaceIndex_1", 1);
    fit.setProperty("MaxIterations", 10);
    fit.setProperty("Output", "fit");
    fit.execute();

    double chi2 = fit.getProperty("OutputChi2overDoF");
    TS_ASSERT_DELTA(chi2, 0.0, 1e-6);

    auto &ads = API::AnalysisDataService::Instance();
    ads.clear();
  }

  void test_fit_ms() {
    std::string fun =
        "name=CrystalFieldFunction,Ions=(Ce, Pr),Symmetries=(C2v, D4h), "
        "FixAllPeaks=1,"
        "Temperatures=4,FWHMs=2.0,ToleranceIntensity=0.02,"
        "ion0.B20=0.37737,ion0.B22=3.9770,ion0.B40=-0.031787,ion0.B42=-0.11611,"
        "ion0.B44=-0.12544,"
        "ion1.B20=0.4268, ion1.B40=0.001031, ion1.B44=-0.01996, "
        "ion1.B60=0.00005, ion1.B64=0.001563,"
        "ties=(ion0.BmolX=0,ion0.BmolY=0,ion0.BmolZ=0,ion0.BextX=0,ion0.BextY="
        "0,ion0.BextZ=0, ion0.B60=0,ion0.B62=0,ion0.B64=0,ion0.B66=0, "
        "ion0.IntensityScaling=1),"
        "ties=(ion1.BmolX=0,ion1.BmolY=0,ion1.BmolZ=0,ion1.BextX=0,ion1.BextY="
        "0,ion1.BextZ=0, ion1.IntensityScaling=1),";

    auto data = makeDataMS();

    Algorithms::Fit fit;
    fit.initialize();
    fit.setProperty("Function", fun);
    fit.setProperty("InputWorkspace", data);
    fit.setProperty("WorkspaceIndex", 1);
    fit.setProperty("MaxIterations", 10);
    fit.setProperty("Output", "fit_ms");
    fit.execute();

    double chi2 = fit.getProperty("OutputChi2overDoF");
    TS_ASSERT_DELTA(chi2, 0.0, 1e-6);

    API::AnalysisDataService::Instance().clear();
  }

  void test_fit_mm() {
    std::string fun =
        "name=CrystalFieldFunction,Ions=(Ce, Pr),Symmetries=(C2v, D4h), "
        "FixAllPeaks=1,"
        "Temperatures=(4, 10),FWHMs=2.0,ToleranceIntensity=0.02,"
        "ion0.B20=0.3773,ion0.B22=3.97,ion0.B40=-0.0317,ion0.B42=-0.116,ion0."
        "B44=-0.125,"
        "ion1.B20=0.42, ion1.B40=0.001, ion1.B44=-0.019, ion1.B60=0.000051, "
        "ion1.B64=0.0015,"
        "ties=(ion0.BmolX=0,ion0.BmolY=0,ion0.BmolZ=0,ion0.BextX=0,ion0.BextY="
        "0,ion0.BextZ=0, ion0.B60=0,ion0.B62=0,ion0.B64=0,ion0.B66=0, "
        "ion0.IntensityScaling=1),"
        "ties=(ion1.BmolX=0,ion1.BmolY=0,ion1.BmolZ=0,ion1.BextX=0,ion1.BextY="
        "0,ion1.BextZ=0, ion1.IntensityScaling=1),"
        "ties=(sp0.IntensityScaling=1, sp1.IntensityScaling=1)";

    auto ws = makeDataMM();
    auto sp0 = boost::dynamic_pointer_cast<MatrixWorkspace>(ws->getItem(0));
    auto sp1 = boost::dynamic_pointer_cast<MatrixWorkspace>(ws->getItem(1));

    Algorithms::Fit fit;
    fit.initialize();
    fit.setProperty("Function", fun);
    fit.setProperty("InputWorkspace", sp0);
    fit.setProperty("WorkspaceIndex", 1);
    fit.setProperty("InputWorkspace_1", sp1);
    fit.setProperty("WorkspaceIndex_1", 1);
    fit.setProperty("MaxIterations", 10);
    fit.setProperty("Output", "fit_mm");
    fit.execute();

    double chi2 = fit.getProperty("OutputChi2overDoF");
    TS_ASSERT_DELTA(chi2, 0.0, 1e-6);

    API::AnalysisDataService::Instance().clear();
  }

  void test_fit_mm_with_background() {
    std::string fun =
        "name=CrystalFieldFunction,Ions=(Ce, Pr),Symmetries=(C2v, D4h), "
        "FixAllPeaks=1,"
        "Temperatures=(4, "
        "10),FWHMs=2.0,ToleranceIntensity=0.001,Background=\"name="
        "LinearBackground,A0=20,\","
        "ion0.B20=0.37737,ion0.B22=3.9770,ion0.B40=-0.031787,ion0.B42=-0.11611,"
        "ion0.B44=-0.12544,"
        "ion1.B20=0.4268, ion1.B40=0.001031, ion1.B44=-0.01996, "
        "ion1.B60=0.00005, ion1.B64=0.001563,"
        "ties=(ion0.BmolX=0,ion0.BmolY=0,ion0.BmolZ=0,ion0.BextX=0,ion0.BextY="
        "0,ion0.BextZ=0, ion0.B60=0,ion0.B62=0,ion0.B64=0,ion0.B66=0, "
        "ion0.IntensityScaling=1),"
        "ties=(ion1.BmolX=0,ion1.BmolY=0,ion1.BmolZ=0,ion1.BextX=0,ion1.BextY="
        "0,ion1.BextZ=0, ion1.IntensityScaling=1),"
        "ties=(sp0.IntensityScaling=1, sp1.IntensityScaling=1),"
        "ties=(sp1.bg.A1 = -sp0.bg.A1)";

    auto ws = makeDataMMwithBackground();
    auto sp0 = boost::dynamic_pointer_cast<MatrixWorkspace>(ws->getItem(0));
    auto sp1 = boost::dynamic_pointer_cast<MatrixWorkspace>(ws->getItem(1));

    Algorithms::Fit fit;
    fit.initialize();
    fit.setProperty("Function", fun);
    fit.setProperty("InputWorkspace", sp0);
    fit.setProperty("WorkspaceIndex", 1);
    fit.setProperty("InputWorkspace_1", sp1);
    fit.setProperty("WorkspaceIndex_1", 1);
    fit.setProperty("MaxIterations", 20);
    fit.setProperty("Output", "fit_mm");
    fit.execute();

    double chi2 = fit.getProperty("OutputChi2overDoF");
    TS_ASSERT_DELTA(chi2, 0.0, 0.1);

    IFunction_sptr function = fit.getProperty("Function");
    auto a1 = function->getParameter("sp0.bg.A1");
    TS_ASSERT_DELTA(a1, -0.1001715899, 1e-3);
    TS_ASSERT_DELTA(a1 + function->getParameter("sp1.bg.A1"), 0.0, 1e-10);

    auto new_fun =
        FunctionFactory::Instance().createInitialized(function->asString());
    TS_ASSERT(new_fun);

    auto &ads = API::AnalysisDataService::Instance();
    ads.clear();
  }

  void test_phys_props_s() {
    auto ws = makeDataSP();
    auto sp0 = boost::dynamic_pointer_cast<MatrixWorkspace>(ws->getItem(0));
    auto sp1 = boost::dynamic_pointer_cast<MatrixWorkspace>(ws->getItem(1));

    std::string fun = "name=CrystalFieldFunction,Ions=Ce,Symmetries=C2v,"
                      "PhysicalProperties=\"cv, chi\","
                      "B20=0.37737,B22=3.9770,chi.Lambda=0.4,"
                      "ties=(B60=0,B62=0,B64=0,B66=0,BmolX=0, "
                      "BmolY=0,BmolZ=0,BextX=0,BextY=0,BextZ=0),"
                      "ties=(B40=-0.031787,B42=-0.11611,B44=-0.12544)";

    Algorithms::Fit fit;
    fit.initialize();
    fit.setProperty("Function", fun);
    fit.setProperty("InputWorkspace", sp0);
    fit.setProperty("WorkspaceIndex", 1);
    fit.setProperty("InputWorkspace_1", sp1);
    fit.setProperty("WorkspaceIndex_1", 1);
    fit.setProperty("MaxIterations", 20);
    fit.setProperty("Output", "fit");
    fit.execute();

    double chi2 = fit.getProperty("OutputChi2overDoF");
    TS_ASSERT_DELTA(chi2, 0.0, 1e-6);

    IFunction_sptr function = fit.getProperty("Function");
    TS_ASSERT_DELTA(function->getParameter("chi.Lambda"), 0.5, 1e-3);

    auto &ads = API::AnalysisDataService::Instance();
    ads.clear();
  }

  void test_phys_props_m() {
    auto ws = makeDataMP();
    auto sp0 = boost::dynamic_pointer_cast<MatrixWorkspace>(ws->getItem(0));
    auto sp1 = boost::dynamic_pointer_cast<MatrixWorkspace>(ws->getItem(1));

    std::string fun =
        "name=CrystalFieldFunction,Ions=\"Ce, Pr\",Symmetries=\"C2v, "
        "D4h\",PhysicalProperties=\"cv, chi\","
        "ion0.B20=0.37737,ion0.B22=3.9770, ion0.B40=-0.031787, "
        "ion0.B42=-0.11611, ion0.B44=-0.12544, ion0.chi.Lambda=0.1,"
        "ion1.B20=0.4268, ion1.B40=0.001031, ion1.B44=-0.01996, "
        "ion1.B60=0.00005, ion1.B64=0.001563, ion1.chi.Lambda=0.2, "
        "ion1.cv.ScaleFactor = 0.1";

    Algorithms::Fit fit;
    fit.initialize();
    fit.setProperty("Function", fun);
    fit.setProperty("InputWorkspace", sp0);
    fit.setProperty("WorkspaceIndex", 1);
    fit.setProperty("InputWorkspace_1", sp1);
    fit.setProperty("WorkspaceIndex_1", 1);
    fit.setProperty("MaxIterations", 20);
    fit.setProperty("Output", "fit");
    fit.execute();

    double chi2 = fit.getProperty("OutputChi2overDoF");
    TS_ASSERT_DELTA(chi2, 0.0, 1e-6);

    IFunction_sptr function = fit.getProperty("Function");
    TS_ASSERT_DELTA(function->getParameter("ion0.chi.Lambda"), 0.0, 1e-3);
    TS_ASSERT_DELTA(function->getParameter("ion1.chi.Lambda"), 0.0, 1e-3);

    auto &ads = API::AnalysisDataService::Instance();
    ads.clear();
  }

  void test_setting_peak_shape_keeps_field_parameters() {
    CrystalFieldFunction cf;
    cf.setAttributeValue("Ions", "Ce, Yb");
    cf.setAttributeValue("Symmetries", "C2v, D6h");
    cf.setAttributeValue("Temperatures", std::vector<double>({44, 50}));
    cf.setAttributeValue("FWHMs", std::vector<double>({1, 2}));
    cf.setParameter("ion0.B20", 1.0);
    cf.setParameter("ion1.B20", 2.0);
    TS_ASSERT_EQUALS(cf.getParameter("ion0.B20"), 1.0);
    TS_ASSERT_EQUALS(cf.getParameter("ion1.B20"), 2.0);
    cf.setAttributeValue("PeakShape", "Lorentzian");
    TS_ASSERT_EQUALS(cf.getParameter("ion0.B20"), 1.0);
    TS_ASSERT_EQUALS(cf.getParameter("ion1.B20"), 2.0);
  }

private:
  MatrixWorkspace_sptr makeDataSS() {
    auto ws = create2DWorkspaceBinned(1, 100, 0.0, 0.5);
    std::string fun = "name=CrystalFieldSpectrum,Ion=Ce,Temperature=44,"
                      "ToleranceIntensity=0.001,B20=0.37737,B22=3.9770,B40=-0."
                      "031787,B42=-0.11611,B44=-0.12544,f0.FWHM=1.6,f1.FWHM=2."
                      "0,f2.FWHM=2.3";
    Algorithms::EvaluateFunction eval;
    eval.initialize();
    eval.setProperty("Function", fun);
    eval.setProperty("InputWorkspace", ws);
    eval.setPropertyValue("OutputWorkspace", "out");
    eval.execute();
    auto out =
        API::AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("out");
    API::AnalysisDataService::Instance().clear();
    return out;
  }

  WorkspaceGroup_sptr makeDataSM() {
    auto ws = create2DWorkspaceBinned(2, 100, 0.0, 0.5);
    std::string fun =
        "name=CrystalFieldMultiSpectrum,Ion=Ce,Temperatures=(10, 50), "
        "FWHMs=(2, 2),"
        "ToleranceIntensity=0.1,"
        "B20=0.37737,B22=3.9770,B40=-0.031787,B42=-0.11611,B44=-0.12544";
    Algorithms::EvaluateFunction eval;
    eval.initialize();
    eval.setProperty("Function", fun);
    eval.setProperty("InputWorkspace", ws);
    eval.setProperty("InputWorkspace_1", ws);
    eval.setProperty("WorkspaceIndex_1", 1);
    eval.setPropertyValue("OutputWorkspace", "out");
    eval.execute();
    auto &ads = API::AnalysisDataService::Instance();
    auto out = ads.retrieveWS<WorkspaceGroup>("out");
    ads.clear();
    return out;
  }

  MatrixWorkspace_sptr makeDataMS() {
    auto ws = create2DWorkspaceBinned(1, 100, 0.0, 0.5);
    std::string fun =
        "name=CrystalFieldSpectrum,Ion=Pr, Symmetry=D4h,Temperature=4, FWHM=2,"
        "ToleranceIntensity=0.001,B20=0.4268, B40=0.001031, B44=-0.01996, "
        "B60=0.00005, B64=0.001563";
    std::string fun1 = "name=CrystalFieldSpectrum,Ion=Ce,Temperature=4, FWHM=2,"
                       "ToleranceIntensity=0.001,B20=0.37737,B22=3.9770,B40=-0."
                       "031787,B42=-0.11611,B44=-0.12544";
    Algorithms::EvaluateFunction eval;
    eval.initialize();
    eval.setProperty("Function", fun + ";" + fun1);
    eval.setProperty("InputWorkspace", ws);
    eval.setPropertyValue("OutputWorkspace", "out");
    eval.execute();
    auto out =
        API::AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("out");
    API::AnalysisDataService::Instance().clear();
    return out;
  }

  WorkspaceGroup_sptr makeDataMM() {
    auto ws = create2DWorkspaceBinned(2, 100, 0.0, 0.5);
    std::string fun = "name=CrystalFieldMultiSpectrum,Ion=Pr, "
                      "Symmetry=D4h,Temperatures=(4, 10), FWHMs=2,"
                      "ToleranceIntensity=0.001,B20=0.4268, B40=0.001031, "
                      "B44=-0.01996, B60=0.00005, B64=0.001563";
    std::string fun1 =
        "name=CrystalFieldMultiSpectrum,Ion=Ce,Temperatures=(4, 10), FWHMs=2,"
        "ToleranceIntensity=0.001,B20=0.37737,B22=3.9770,B40=-0.031787,B42=-0."
        "11611,B44=-0.12544";
    Algorithms::EvaluateFunction eval;
    eval.initialize();
    eval.setProperty("Function", fun + ";" + fun1);
    eval.setProperty("InputWorkspace", ws);
    eval.setProperty("InputWorkspace_1", ws);
    eval.setProperty("WorkspaceIndex_1", 1);
    eval.setPropertyValue("OutputWorkspace", "out");
    eval.execute();
    auto &ads = API::AnalysisDataService::Instance();
    auto out = ads.retrieveWS<WorkspaceGroup>("out");
    ads.clear();
    return out;
  }

  WorkspaceGroup_sptr makeDataMMwithBackground() {
    auto ws = create2DWorkspaceBinned(2, 100, 0.0, 0.5);
    std::string fun1 = "name=CrystalFieldMultiSpectrum,Ion=Pr, "
                       "Symmetry=D4h,Temperatures=(4, 10), FWHMs=2,"
                       "ToleranceIntensity=0.001,B20=0.4268, B40=0.001031, "
                       "B44=-0.01996, B60=0.00005, B64=0.001563";
    std::string fun =
        "name=CrystalFieldMultiSpectrum,Ion=Ce,Temperatures=(4, 10), "
        "FWHMs=2,Background=\"name=LinearBackground,A0=20,A1=-0.11\","
        "ToleranceIntensity=0.001,B20=0.37737,B22=3.9770,B40=-0.031787,B42=-0."
        "11611,B44=-0.12544,f1.f0.A0=10,f1.f0.A1=0.09";
    Algorithms::EvaluateFunction eval;
    eval.initialize();
    eval.setProperty("Function", fun + ";" + fun1);
    eval.setProperty("InputWorkspace", ws);
    eval.setProperty("InputWorkspace_1", ws);
    eval.setProperty("WorkspaceIndex_1", 1);
    eval.setPropertyValue("OutputWorkspace", "out");
    eval.execute();
    auto &ads = API::AnalysisDataService::Instance();
    auto out = ads.retrieveWS<WorkspaceGroup>("out");
    ads.clear();
    return out;
  }

  WorkspaceGroup_sptr makeDataSP() {
    auto ws0 = create2DWorkspaceBinned(1, 100, 0.0, 0.5);
    auto ws1 = create2DWorkspaceBinned(1, 100, 0.0, 0.01);
    std::string fun = "name=CrystalFieldFunction,Ions=Ce,Symmetries=C2v,"
                      "PhysicalProperties=\"cv, chi\","
                      "B20=0.37737,B22=3.9770,B40=-0.031787,B42=-0.11611,B44=-"
                      "0.12544, chi.Lambda=0.5";
    Algorithms::EvaluateFunction eval;
    eval.initialize();
    eval.setProperty("Function", fun);
    eval.setProperty("InputWorkspace", ws0);
    eval.setProperty("InputWorkspace_1", ws1);
    eval.setPropertyValue("OutputWorkspace", "out");
    eval.execute();
    auto &ads = API::AnalysisDataService::Instance();
    auto out = ads.retrieveWS<WorkspaceGroup>("out");
    ads.clear();
    return out;
  }

  WorkspaceGroup_sptr makeDataMP() {
    auto ws0 = create2DWorkspaceBinned(1, 100, 0.0, 0.5);
    auto ws1 = create2DWorkspaceBinned(1, 100, 0.0, 0.3);
    std::string fun =
        "name=CrystalFieldFunction,Ions=\"Ce, Pr\",Symmetries=\"C2v, "
        "D4h\",PhysicalProperties=\"cv, chi\","
        "ion0.B20=0.37737,ion0.B22=3.9770, ion0.B40=-0.031787, "
        "ion0.B42=-0.11611, ion0.B44=-0.12544, ion0.chi.Lambda=0.,"
        "ion1.B20=0.4268, ion1.B40=0.001031, ion1.B44=-0.01996, "
        "ion1.B60=0.00005, ion1.B64=0.001563, ion1.chi.Lambda=0., "
        "ion1.cv.ScaleFactor = 0.1";
    Algorithms::EvaluateFunction eval;
    eval.initialize();
    eval.setProperty("Function", fun);
    eval.setProperty("InputWorkspace", ws0);
    eval.setProperty("InputWorkspace_1", ws1);
    eval.setPropertyValue("OutputWorkspace", "out");
    eval.execute();
    auto &ads = API::AnalysisDataService::Instance();
    auto out = ads.retrieveWS<WorkspaceGroup>("out");
    ads.clear();
    return out;
  }
};

#endif /*CRYSTALFIELDFUNCTIONTEST_H_*/
