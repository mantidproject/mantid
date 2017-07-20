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

  void xtest_names_ss() {
    CrystalFieldFunction cf;
    cf.setAttributeValue("Ions", "Ce");
    cf.setAttributeValue("Symmetries", "C2v");
    cf.setAttributeValue("Temperatures", std::vector<double>({44}));
    cf.setAttributeValue("FWHMs", std::vector<double>({1}));
    auto fun = FunctionFactory::Instance().createInitialized(cf.asString());
    TS_ASSERT_EQUALS(fun->getAttribute("Ions").asUnquotedString(), "Ce");
    TS_ASSERT_EQUALS(fun->getAttribute("Symmetries").asUnquotedString(), "C2v");
    TS_ASSERT_EQUALS(fun->getAttribute("Temperatures").asVector(), std::vector<double>({44}));
    TS_ASSERT_EQUALS(fun->getAttribute("FWHMs").asVector(), std::vector<double>({1}));
    auto attributeNames = cf.getAttributeNames();
    TS_ASSERT_EQUALS(attributeNames.size(), 15);
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
    TS_ASSERT_EQUALS(attributeNames[0], "Ions");
    TS_ASSERT_EQUALS(attributeNames[1], "Symmetries");
    TS_ASSERT_EQUALS(attributeNames[2], "Temperatures");

    for(size_t i = 33; i < cf.nParams(); ++i) {
      std::cerr << i << ' ' << cf.parameterName(i) << std::endl;
    }

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

    //auto fun = FunctionFactory::Instance().createInitialized(cf.asString());
    //TS_ASSERT_EQUALS(fun->getAttribute("Ions").asUnquotedString(), "Ce");
    //TS_ASSERT_EQUALS(fun->getAttribute("Symmetries").asUnquotedString(), "C2v");
    //TS_ASSERT_EQUALS(fun->getAttribute("Temperatures").asVector(), std::vector<double>({44}));
    //TS_ASSERT_EQUALS(fun->getAttribute("FWHMs").asVector(), std::vector<double>({1}));
  }

  void xtest_names_ms() {
    CrystalFieldFunction cf;
    cf.setAttributeValue("Ions", "Ce, Yb");
    cf.setAttributeValue("Symmetries", "C2v, D6h");
    cf.setAttributeValue("Temperatures", std::vector<double>({44}));
    cf.setAttributeValue("FWHMs", std::vector<double>({1}));
    auto attributeNames = cf.getAttributeNames();
    TS_ASSERT_EQUALS(attributeNames.size(), 15);
    TS_ASSERT_EQUALS(attributeNames[0], "Ions");
    TS_ASSERT_EQUALS(attributeNames[1], "Symmetries");
    TS_ASSERT_EQUALS(attributeNames[2], "Temperatures");

    //for(size_t i = 0; i < cf.nParams(); ++i) {
    //  std::cerr << i << ' ' << cf.parameterName(i) << std::endl;
    //}
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
};

#endif /*CRYSTALFIELDFUNCTIONTEST_H_*/
