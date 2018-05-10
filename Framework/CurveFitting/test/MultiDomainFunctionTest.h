#ifndef MULTIDOMAINFUNCTIONTEST_H_
#define MULTIDOMAINFUNCTIONTEST_H_

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/FunctionDomain1D.h"
#include "MantidAPI/FunctionValues.h"
#include "MantidCurveFitting/Algorithms/Fit.h"
#include "MantidCurveFitting/CostFunctions/CostFuncLeastSquares.h"
#include "MantidCurveFitting/FuncMinimizers/LevenbergMarquardtMDMinimizer.h"

#include "MantidTestHelpers/FakeObjects.h"
#include "MantidTestHelpers/MultiDomainFunctionHelper.h"

#include <cxxtest/TestSuite.h>

#include <algorithm>
#include <boost/make_shared.hpp>

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::CurveFitting;
using namespace Mantid::CurveFitting::CostFunctions;
using namespace Mantid::CurveFitting::Algorithms;

using Mantid::TestHelpers::MultiDomainFunctionTest_Function;

class MultiDomainFunctionTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MultiDomainFunctionTest *createSuite() {
    return new MultiDomainFunctionTest();
  }
  static void destroySuite(MultiDomainFunctionTest *suite) { delete suite; }

  MultiDomainFunctionTest() { FrameworkManager::Instance(); }

  void test_multidomain() {
    boost::shared_ptr<JointDomain> domain;
    TS_ASSERT_THROWS_NOTHING(domain =
                                 Mantid::TestHelpers::makeMultiDomainDomain3());

    auto values = boost::make_shared<FunctionValues>(*domain);
    const double A0 = 0, A1 = 1, A2 = 2;
    const double B0 = 1, B1 = 2, B2 = 3;

    auto &d0 = static_cast<const FunctionDomain1D &>(domain->getDomain(0));
    for (size_t i = 0; i < d0.size(); ++i) {
      values->setFitData(i, A0 + A1 + A2 + (B0 + B1 + B2) * d0[i]);
    }

    auto &d1 = static_cast<const FunctionDomain1D &>(domain->getDomain(1));
    for (size_t i = 0; i < d1.size(); ++i) {
      values->setFitData(9 + i, A0 + A1 + (B0 + B1) * d1[i]);
    }

    auto &d2 = static_cast<const FunctionDomain1D &>(domain->getDomain(2));
    for (size_t i = 0; i < d2.size(); ++i) {
      values->setFitData(19 + i, A0 + A2 + (B0 + B2) * d2[i]);
    }
    values->setFitWeights(1);

    boost::shared_ptr<MultiDomainFunction> multi;
    TS_ASSERT_THROWS_NOTHING(
        multi = Mantid::TestHelpers::makeMultiDomainFunction3());
  }
};

#endif /*MULTIDOMAINFUNCTIONTEST_H_*/
