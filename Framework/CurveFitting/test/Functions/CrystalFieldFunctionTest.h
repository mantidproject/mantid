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

  void test_stuff() {
  }

  void test_string() {
    CrystalFieldFunction cf;
    cf.setAttributeValue("Ions", "Ce");
    cf.setAttributeValue("Symmetries", "C2v");
    cf.setAttributeValue("Temperatures", std::vector<double>({44}));
    cf.setAttributeValue("FWHMs", std::vector<double>({1}));
    auto fun = FunctionFactory::Instance().createInitialized(cf.asString());
    auto attributeNames = cf.getAttributeNames();
    for(auto name: attributeNames) {
      std::cerr << name << std::endl;
    }
    std::cerr << "---------------------------------\n";
    for(size_t i = 0; i < cf.nParams(); ++i) {
      std::cerr << i << ' ' << cf.parameterName(i) << std::endl;
    }
  }
};

#endif /*CRYSTALFIELDFUNCTIONTEST_H_*/
