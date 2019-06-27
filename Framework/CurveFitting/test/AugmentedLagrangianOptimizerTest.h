// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_KERNEL_AUGMENTEDLAGRANGIANOPTIMIZERTEST_H_
#define MANTID_KERNEL_AUGMENTEDLAGRANGIANOPTIMIZERTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/AugmentedLagrangianOptimizer.h"
#include "MantidKernel/Matrix.h"

#include <boost/bind.hpp>
#include <boost/make_shared.hpp>

class AugmentedLagrangianOptimizerTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static AugmentedLagrangianOptimizerTest *createSuite() {
    return new AugmentedLagrangianOptimizerTest();
  }
  static void destroySuite(AugmentedLagrangianOptimizerTest *suite) {
    delete suite;
  }

  enum CONSTRAINT_TYPE {
    NOCONSTRAINTS,
    EMPTYCONSTRAINTS,
    EQUALITYCONSTRAINT,
    INEQUALITYCONSTRAINT,
    BOTHCONSTRAINTS
  };

  AugmentedLagrangianOptimizerTest() : CxxTest::TestSuite(), m_nparams(2) {}

  void
  test_constuctor_with_equality_matrix_whose_num_columns_dont_match_nparams_throws() {
    using Mantid::CurveFitting::AugmentedLagrangianOptimizer;
    using Mantid::Kernel::DblMatrix;

    const size_t nparams(2);
    DblMatrix equality(1, nparams + 1); // cols > number parameters
    DblMatrix inequality;               // Empty indicates no constraint

    AugmentedLagrangianOptimizer::ObjFunction userFunc;
    TS_ASSERT_THROWS(
        AugmentedLagrangianOptimizer(nparams, userFunc, equality, inequality),
        const std::invalid_argument &);

    equality = DblMatrix(1, nparams - 1); // cols < number parameters
    TS_ASSERT_THROWS(
        AugmentedLagrangianOptimizer(nparams, userFunc, equality, inequality),
        const std::invalid_argument &);
  }

  void
  test_constuctor_with_inequality_matrix_whose_num_columns_dont_match_nparams_throws() {
    using Mantid::CurveFitting::AugmentedLagrangianOptimizer;
    using Mantid::Kernel::DblMatrix;

    const size_t nparams(2);
    DblMatrix equality; // Empty indicates no constraint
    DblMatrix inequality(1, nparams + 1);

    AugmentedLagrangianOptimizer::ObjFunction userFunc;
    TS_ASSERT_THROWS(
        AugmentedLagrangianOptimizer(nparams, userFunc, equality, inequality),
        const std::invalid_argument &);

    inequality = DblMatrix(1, nparams - 1); // cols < number parameters
    TS_ASSERT_THROWS(
        AugmentedLagrangianOptimizer(nparams, userFunc, equality, inequality),
        const std::invalid_argument &);
  }

  void test_minimizer_calls_user_function() {
    using Mantid::CurveFitting::AugmentedLagrangianOptimizer;

    bool userFuncCalled = false;
    TestUserFuncCall testFunc(userFuncCalled);
    AugmentedLagrangianOptimizer::ObjFunction userFunc =
        boost::bind(&TestUserFuncCall::eval, testFunc, _1, _2);
    AugmentedLagrangianOptimizer lsqmin(2, userFunc);

    std::vector<double> xv(2, 1);
    lsqmin.minimize(xv);

    TS_ASSERT(userFuncCalled);
  }

  //---------------------------------------------------------------------------
  // The following tests are taken from scipy/optimize/tests.test_slsq.py
  //---------------------------------------------------------------------------

  void test_minimize_with_no_constraints_specified() {
    auto res = runMinimizer(NOCONSTRAINTS);
    TS_ASSERT_EQUALS(m_nparams, res.size());
    if (res.size() == m_nparams) {
      TS_ASSERT_DELTA(2.0, res[0], 1e-7);
      TS_ASSERT_DELTA(1.0, res[1], 1e-7);
    }
  }

  void test_minimize_with_empty_constraints_gives_same_as_no_constraints() {
    auto res = runMinimizer(EMPTYCONSTRAINTS);
    TS_ASSERT_EQUALS(m_nparams, res.size());
    if (res.size() == m_nparams) {
      TS_ASSERT_DELTA(2.0, res[0], 1e-7);
      TS_ASSERT_DELTA(1.0, res[1], 1e-7);
    }
  }

  void test_minimize_with_equality_constraint() {
    auto res = runMinimizer(EQUALITYCONSTRAINT);
    TS_ASSERT_EQUALS(m_nparams, res.size());
    if (res.size() == m_nparams) {
      TS_ASSERT_DELTA(1.0, res[0], 1e-5);
      TS_ASSERT_DELTA(1.0, res[1], 1e-5);
    }
  }

  void test_minimize_with_inequality_constraint() {
    auto res = runMinimizer(INEQUALITYCONSTRAINT);
    TS_ASSERT_EQUALS(m_nparams, res.size());
    if (res.size() == m_nparams) {
      TS_ASSERT_DELTA(1.4709132, res[0], 1e-7);
      TS_ASSERT_DELTA(0.2941826, res[1], 1e-7);
    }
  }

  void test_minimize_with_both_equality_and_inequality_constraint() {
    using std::abs;
    auto res = runMinimizer(BOTHCONSTRAINTS);
    TS_ASSERT_EQUALS(m_nparams, res.size());
    if (res.size() == m_nparams) {
      TS_ASSERT_DELTA(0.0, abs(res[0]), 1e-6);
      TS_ASSERT_DELTA(0.0, abs(res[1]), 1e-6);
    }
  }

private:
  std::vector<double> runMinimizer(const CONSTRAINT_TYPE type) {
    using Mantid::CurveFitting::AugmentedLagrangianOptimizer;
    using Mantid::Kernel::DblMatrix;

    const size_t nparams = m_nparams;
    AugmentedLagrangianOptimizer::ObjFunction userFunc = &TestObjFunction::eval;

    std::vector<double> xv(nparams, -1);
    xv[1] = 1.0;

    // x-y==0 ==> [1 -1][x   == 0
    //                  y]
    DblMatrix equality(1, nparams);
    equality[0][0] = 1.0;
    equality[0][1] = -1.0;

    // x-5y>=0 ==> [-1 5][x   <= 0
    //                  y]
    DblMatrix inequality(1, nparams);
    inequality[0][0] = -1.0;
    inequality[0][1] = 5.0;

    boost::shared_ptr<AugmentedLagrangianOptimizer> lsqmin;
    switch (type) {
    case NOCONSTRAINTS:
      lsqmin =
          boost::make_shared<AugmentedLagrangianOptimizer>(nparams, userFunc);
      break;
    case EMPTYCONSTRAINTS:
      lsqmin = boost::make_shared<AugmentedLagrangianOptimizer>(
          nparams, userFunc, DblMatrix(), DblMatrix());
      break;
    case EQUALITYCONSTRAINT:
      lsqmin = boost::make_shared<AugmentedLagrangianOptimizer>(
          nparams, userFunc, equality, DblMatrix());
      break;
    case INEQUALITYCONSTRAINT:
      lsqmin = boost::make_shared<AugmentedLagrangianOptimizer>(
          nparams, userFunc, DblMatrix(), inequality);
      break;
    case BOTHCONSTRAINTS:
      lsqmin = boost::make_shared<AugmentedLagrangianOptimizer>(
          nparams, userFunc, equality, inequality);
      break;
    };

    lsqmin->minimize(xv);
    return xv;
  }

  struct TestUserFuncCall {
    TestUserFuncCall(bool &flag) : funcCalled(flag) {}
    double eval(const size_t, const double *) const {
      funcCalled = true;
      return 0.0;
    }
    bool &funcCalled;
  };

  struct TestObjFunction {
    static double eval(const size_t, const double *xpt) {
      // evaluates f(x) = 2*x*y + 2*x - x**2 - 2*y**2
      const double x(xpt[0]), y(xpt[1]);
      return -1.0 * (2.0 * x * y + 2 * x - x * x - 2 * y * y);
    }
  };

  const size_t m_nparams;
};

#endif /* MANTID_KERNEL_UGMENTEDLAGRANGIANOPTIMIZERTEST_H_ */
