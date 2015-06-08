#ifndef MANTID_CURVEFITTING_EVALUATEFUNCTIONTEST_H_
#define MANTID_CURVEFITTING_EVALUATEFUNCTIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/EvaluateFunction.h"
#include "MantidAPI/FunctionDomain1D.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/FunctionValues.h"
#include "MantidAPI/IFunction1D.h"
#include "MantidKernel/EmptyValues.h"

#include <algorithm>
#include <boost/math/special_functions/fpclassify.hpp>
#include <limits>

using Mantid::CurveFitting::EvaluateFunction;
using namespace Mantid;
using namespace Mantid::API;

class EvaluateFunctionTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static EvaluateFunctionTest *createSuite() {
    return new EvaluateFunctionTest();
  }
  static void destroySuite(EvaluateFunctionTest *suite) { delete suite; }

  void test_Init() {
    EvaluateFunction alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

};

#endif /* MANTID_CURVEFITTING_EVALUATEFUNCTIONTEST_H_ */
