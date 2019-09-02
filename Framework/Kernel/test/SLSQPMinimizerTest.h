// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_KERNEL_SLSQPMINIMIZERTEST_H_
#define MANTID_KERNEL_SLSQPMINIMIZERTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidKernel/Math/Optimization/SLSQPMinimizer.h"
#include "MantidKernel/Matrix.h"

#include <boost/shared_ptr.hpp>

class SLSQPMinimizerTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SLSQPMinimizerTest *createSuite() { return new SLSQPMinimizerTest(); }
  static void destroySuite(SLSQPMinimizerTest *suite) { delete suite; }

  enum CONSTRAINT_TYPE {
    NOCONSTRAINTS,
    EMPTYCONSTRAINTS,
    EQUALITYCONSTRAINT,
    INEQUALITYCONSTRAINT,
    BOTHCONSTRAINTS
  };

  SLSQPMinimizerTest() : CxxTest::TestSuite(), m_nparams(2) {}

  void
  test_constuctor_with_equality_matrix_whose_num_columns_dont_match_nparams_throws() {
    using Mantid::Kernel::DblMatrix;
    using Mantid::Kernel::Math::SLSQPMinimizer;

    const size_t nparams(2);
    DblMatrix equality(1, nparams + 1); // cols > number parameters
    DblMatrix inequality;               // Empty indicates no constraint

    ObjFunction userFunc;
    TS_ASSERT_THROWS(SLSQPMinimizer(nparams, userFunc, equality, inequality),
                     const std::invalid_argument &);

    equality = DblMatrix(1, nparams - 1); // cols < number parameters
    TS_ASSERT_THROWS(SLSQPMinimizer(nparams, userFunc, equality, inequality),
                     const std::invalid_argument &);
  }

  void
  test_constuctor_with_inequality_matrix_whose_num_columns_dont_match_nparams_throws() {
    using Mantid::Kernel::DblMatrix;
    using Mantid::Kernel::Math::SLSQPMinimizer;

    const size_t nparams(2);
    DblMatrix equality; // Empty indicates no constraint
    DblMatrix inequality(1, nparams + 1);

    ObjFunction userFunc;
    TS_ASSERT_THROWS(SLSQPMinimizer(nparams, userFunc, equality, inequality),
                     const std::invalid_argument &);

    inequality = DblMatrix(1, nparams - 1); // cols < number parameters
    TS_ASSERT_THROWS(SLSQPMinimizer(nparams, userFunc, equality, inequality),
                     const std::invalid_argument &);
  }

  void test_minimizer_calls_user_function() {
    using Mantid::Kernel::Math::SLSQPMinimizer;

    bool userFuncCalled = false;
    TestUserFuncCall userFunc(userFuncCalled);
    SLSQPMinimizer lsqmin(2, userFunc);

    std::vector<double> start(2, 1);
    std::vector<double> res = lsqmin.minimize(start);

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
      TS_ASSERT_DELTA(1.0, res[0], 1e-7);
      TS_ASSERT_DELTA(1.0, res[1], 1e-7);
    }
  }

  void test_minimize_with_inequality_constraint() {
    auto res = runMinimizer(INEQUALITYCONSTRAINT);
    TS_ASSERT_EQUALS(m_nparams, res.size());
    if (res.size() == m_nparams) {
      TS_ASSERT_DELTA(1.46973601, res[0], 1e-7);
      TS_ASSERT_DELTA(0.2939472, res[1], 1e-7);
    }
  }

  void test_minimize_with_both_equality_and_inequality_constraint() {
    auto res = runMinimizer(BOTHCONSTRAINTS);
    TS_ASSERT_EQUALS(m_nparams, res.size());
    if (res.size() == m_nparams) {
      TS_ASSERT_DELTA(0.0, res[0], 1e-8);
      TS_ASSERT_DELTA(0.0, res[1], 1e-8);
    }
  }

private:
  std::vector<double> runMinimizer(const CONSTRAINT_TYPE type) {
    using Mantid::Kernel::DblMatrix;
    using Mantid::Kernel::Math::SLSQPMinimizer;

    auto lsqmin = boost::shared_ptr<SLSQPMinimizer>();
    const size_t nparams = m_nparams;
    ObjFunction userFunc;
    std::vector<double> start(nparams, -1);
    start[1] = 1.0;

    // x-y>=0 ==> [1 -1][x   >= 0
    //                  y]
    DblMatrix equality(1, nparams);
    equality[0][0] = 1.0;
    equality[0][1] = -1.0;

    // x-5y>=0 ==> [1 -5][x   >= 0
    //                  y]
    DblMatrix inequality(1, nparams);
    inequality[0][0] = 1.0;
    inequality[0][1] = -5.0;

    switch (type) {
    case NOCONSTRAINTS:
      lsqmin = boost::shared_ptr<SLSQPMinimizer>(
          new SLSQPMinimizer(nparams, userFunc));
      break;
    case EMPTYCONSTRAINTS:
      lsqmin = boost::shared_ptr<SLSQPMinimizer>(
          new SLSQPMinimizer(nparams, userFunc, DblMatrix(), DblMatrix()));
      break;
    case EQUALITYCONSTRAINT:
      lsqmin = boost::shared_ptr<SLSQPMinimizer>(
          new SLSQPMinimizer(nparams, userFunc, equality, DblMatrix()));
      break;
    case INEQUALITYCONSTRAINT:
      lsqmin = boost::shared_ptr<SLSQPMinimizer>(
          new SLSQPMinimizer(nparams, userFunc, DblMatrix(), inequality));
      break;
    case BOTHCONSTRAINTS:
      lsqmin = boost::shared_ptr<SLSQPMinimizer>(
          new SLSQPMinimizer(nparams, userFunc, equality, inequality));
      break;
    };

    return lsqmin->minimize(start);
  }

  struct TestUserFuncCall {
    TestUserFuncCall(bool &flag) : funcCalled(flag) {}
    double eval(const std::vector<double> &) const {
      funcCalled = true;
      return 0.0;
    }
    bool &funcCalled;
  };

  struct ObjFunction {
    double eval(const std::vector<double> &xpt) const {
      // evaluates f(x) = 2*x*y + 2*x - x**2 - 2*y**2
      const double x(xpt[0]), y(xpt[1]);
      return -1.0 * (2.0 * x * y + 2 * x - x * x - 2 * y * y);
    }
  };

  const size_t m_nparams;
};

#endif /* MANTID_KERNEL_SLSQPMINIMIZERTEST_H_ */
