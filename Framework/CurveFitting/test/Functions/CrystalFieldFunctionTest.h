#ifndef CRYSTALFIELDFUNCTIONTEST_H_
#define CRYSTALFIELDFUNCTIONTEST_H_

#include <cxxtest/TestSuite.h>

//#include "MantidAPI/FunctionDomain1D.h"
//#include "MantidAPI/FunctionValues.h"
#include "MantidAPI/FunctionFactory.h"
//#include "MantidAPI/ParameterTie.h"
#include "MantidCurveFitting/Functions/CrystalFieldFunction.h"

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::CurveFitting;
using namespace Mantid::CurveFitting::Functions;

class CrystalFieldFunctionTest : public CxxTest::TestSuite {
public:

  void test_names_ss() {
    CrystalFieldFunction cf;
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
    TS_ASSERT_EQUALS(fun->getAttribute("Temperatures").asVector(), std::vector<double>({44}));
    TS_ASSERT_EQUALS(fun->getAttribute("FWHMs").asVector(), std::vector<double>({1}));
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
    TS_ASSERT_EQUALS(fun->getAttribute("Temperatures").asVector(), std::vector<double>({44}));
    TS_ASSERT_EQUALS(fun->getAttribute("FWHMs").asVector(), std::vector<double>({1}));
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
    TS_ASSERT_EQUALS(fun->getAttribute("Temperatures").asVector(), std::vector<double>({44, 50}));
    TS_ASSERT_EQUALS(fun->getAttribute("FWHMs").asVector(), std::vector<double>({1, 2}));
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
    TS_ASSERT_EQUALS(fun->getAttribute("Temperatures").asVector(), std::vector<double>({44, 50}));
    TS_ASSERT_EQUALS(fun->getAttribute("FWHMs").asVector(), std::vector<double>({1, 2}));
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
    TS_ASSERT_EQUALS(fun->getAttribute("Symmetries").asUnquotedString(), "C2v,D6h");
    TS_ASSERT_EQUALS(fun->getAttribute("Temperatures").asVector(), std::vector<double>({44}));
    TS_ASSERT_EQUALS(fun->getAttribute("FWHMs").asVector(), std::vector<double>({1}));

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
    TS_ASSERT_EQUALS(fun->getAttribute("Symmetries").asUnquotedString(), "C2v,D6h");
    TS_ASSERT_EQUALS(fun->getAttribute("Temperatures").asVector(), std::vector<double>({44}));
    TS_ASSERT_EQUALS(fun->getAttribute("FWHMs").asVector(), std::vector<double>({1}));

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
    TS_ASSERT_EQUALS(fun->getAttribute("Symmetries").asUnquotedString(), "C2v,D6h");
    TS_ASSERT_EQUALS(fun->getAttribute("Temperatures").asVector(), std::vector<double>({44, 50}));
    TS_ASSERT_EQUALS(fun->getAttribute("FWHMs").asVector(), std::vector<double>({1, 2}));

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
    TS_ASSERT_EQUALS(fun->getAttribute("Symmetries").asUnquotedString(), "C2v,D6h");
    TS_ASSERT_EQUALS(fun->getAttribute("Temperatures").asVector(), std::vector<double>({44, 50}));
    TS_ASSERT_EQUALS(fun->getAttribute("FWHMs").asVector(), std::vector<double>({1, 2}));

  }
};

#endif /*CRYSTALFIELDFUNCTIONTEST_H_*/
