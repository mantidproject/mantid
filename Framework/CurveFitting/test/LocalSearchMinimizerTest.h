#ifndef MANTID_CURVEFITTING_LOCALSEARCHMINIMIZERTEST_H_
#define MANTID_CURVEFITTING_LOCALSEARCHMINIMIZERTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/FuncMinimizers/LocalSearchMinimizer.h"
#include "MantidCurveFitting/CostFunctions/CostFuncLeastSquares.h"
#include "MantidAPI/FunctionDomain1D.h"
#include "MantidAPI/FunctionValues.h"

using Mantid::CurveFitting::FuncMinimisers::LocalSearchMinimizer;
using Mantid::CurveFitting::CostFunctions::CostFuncLeastSquares;
using Mantid::API::FunctionDomain1D;
using Mantid::API::FunctionValues;

class LocalSearchMinimizerTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static LocalSearchMinimizerTest *createSuite() { return new LocalSearchMinimizerTest(); }
  static void destroySuite( LocalSearchMinimizerTest *suite ) { delete suite; }


  void test_Something()
  {
    TS_FAIL( "You forgot to write a test!");
  }


};


#endif /* MANTID_CURVEFITTING_LOCALSEARCHMINIMIZERTEST_H_ */
