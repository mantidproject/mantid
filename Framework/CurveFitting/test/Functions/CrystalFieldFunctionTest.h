#ifndef CRYSTALFIELDFUNCTIONTEST_H_
#define CRYSTALFIELDFUNCTIONTEST_H_

#include <cxxtest/TestSuite.h>

//#include "MantidAPI/FunctionDomain1D.h"
//#include "MantidAPI/FunctionValues.h"
//#include "MantidAPI/FunctionFactory.h"
//#include "MantidAPI/ParameterTie.h"
#include "MantidCurveFitting/Functions/CrystalFieldFunction.h"

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::CurveFitting;
using namespace Mantid::CurveFitting::Functions;

class CrystalFieldFunctionTest : public CxxTest::TestSuite {
public:

  void test_attributes() {
    CrystalFieldFunction fun;
    TS_ASSERT_THROWS(fun.checkConsistent(), std::runtime_error);

    fun.setAttributeValue("Ions", "Ce ");
    std::string ions = fun.getAttribute("Ions").asString();
    TS_ASSERT_EQUALS(ions, "Ce");
    TS_ASSERT_THROWS(fun.checkConsistent(), std::runtime_error);
  }

};

#endif /*CRYSTALFIELDFUNCTIONTEST_H_*/
