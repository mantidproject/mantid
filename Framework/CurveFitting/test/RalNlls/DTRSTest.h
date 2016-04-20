#ifndef CURVEFITTING_RAL_NLLS_DTRSTEST_H_
#define CURVEFITTING_RAL_NLLS_DTRSTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/RalNlls/DTRS.h"
#include "MantidCurveFitting/Functions/SimpleChebfun.h"

#include <iostream>

using namespace Mantid::CurveFitting::RalNlls;
using namespace Mantid::CurveFitting::Functions;
using namespace Mantid::CurveFitting;

class DTRSTest : public CxxTest::TestSuite {
public:
  void xtest_sign() {
    TS_ASSERT_EQUALS(sign(12.0, 1.0), 12.0);
    TS_ASSERT_EQUALS(sign(12.0, 0.0), 12.0);
    TS_ASSERT_EQUALS(sign(12.0, -1.0), -12.0);
    TS_ASSERT_EQUALS(sign(-12.0, 1.0), 12.0);
    TS_ASSERT_EQUALS(sign(-12.0, 0.0), 12.0);
    TS_ASSERT_EQUALS(sign(-12.0, -1.0), -12.0);
  }

  void xtest_roots_quadratic() {
    double root1 = 0.0, root2 = 0.0;
    int nroots = -1;
    const double tol = 1e-15;
    roots_quadratic(2.0, 2.0, 1.0, tol, nroots, root1, root2);
    TS_ASSERT_EQUALS(nroots, 0);

    roots_quadratic(-2.0, -2.0, -1.0, 1e-15, nroots, root1, root2);
    TS_ASSERT_EQUALS(nroots, 0);

    roots_quadratic(2.0, 0.0, 0.0, 1e-15, nroots, root1, root2);
    TS_ASSERT_EQUALS(nroots, 0);

    root1 = 100.0;
    roots_quadratic(0.0, 0.0, 0.0, 1e-15, nroots, root1, root2);
    TS_ASSERT_EQUALS(nroots, 1);
    TS_ASSERT_DELTA(root1, 0.0, tol);

    roots_quadratic(2.0, -1.0, 0.0, 1e-15, nroots, root1, root2);
    TS_ASSERT_EQUALS(nroots, 1);
    TS_ASSERT_DELTA(root1, 2.0, tol);

    roots_quadratic(-1.0, 0.0, 1.0, 1e-15, nroots, root1, root2);
    TS_ASSERT_EQUALS(nroots, 2);
    TS_ASSERT_DELTA(root1, -1.0, tol);
    TS_ASSERT_DELTA(root2, 1.0, tol);

    roots_quadratic(0.0, 0.0, 10.0, 1e-15, nroots, root1, root2);
    // Repeated roots sould be considered as the same root (probably).
    TS_ASSERT_EQUALS(nroots, 2);
    TS_ASSERT_DELTA(root1, 0.0, tol);
    TS_ASSERT_DELTA(root2, 0.0, tol);

    roots_quadratic(1.0, 2.0, 1.0, 1e-15, nroots, root1, root2);
    TS_ASSERT_EQUALS(nroots, 2);
    TS_ASSERT_DELTA(root1, -1.0, tol);
    TS_ASSERT_DELTA(root2, -1.0, tol);
  }

  void xtest_roots_cubic() {
    double root1 = 0.0, root2 = 0.0, root3 = 0.0;
    int nroots = -1;
    const double tol = 1e-15;

    double a0, a1, a2, a3;
    auto cubic = [&](double x){return a3*x*x*x + a2*x*x + a1*x + a0;};

    std::tie(a0, a1, a2, a3) = std::make_tuple(-1.0, -1.0, 1.0, 1.0);
    roots_cubic(a0, a1, a2, a3, 1e-15, nroots, root1, root2, root3);

    TS_ASSERT_EQUALS(nroots, 3);
    TS_ASSERT_DELTA(root1, -1.0, tol);
    TS_ASSERT_DELTA(root2, -1.0, tol);
    TS_ASSERT_DELTA(root3, 1.0, tol);
    TS_ASSERT_DELTA(cubic(root1), 0.0, tol);
    TS_ASSERT_DELTA(cubic(root2), 0.0, tol);
    TS_ASSERT_DELTA(cubic(root3), 0.0, tol);

    std::tie(a0, a1, a2, a3) = std::make_tuple(-15.0, -2.0, 5.0, 2.0);
    roots_cubic(a0, a1, a2, a3, 1e-15, nroots, root1, root2, root3);

    TS_ASSERT_EQUALS(nroots, 1);
    TS_ASSERT_DELTA(root1, 1.5, tol);
    TS_ASSERT_DELTA(cubic(root1), 0.0, tol);

    std::tie(a0, a1, a2, a3) = std::make_tuple(-6.0, -7.0, 0.0, 1.0);
    roots_cubic(a0, a1, a2, a3, 1e-15, nroots, root1, root2, root3);

    TS_ASSERT_EQUALS(nroots, 3);
    TS_ASSERT_DELTA(root1, -2.0, tol);
    TS_ASSERT_DELTA(root2, -1.0, tol);
    TS_ASSERT_DELTA(root3, 3.0, tol);
    TS_ASSERT_DELTA(cubic(root1), 0.0, tol);
    TS_ASSERT_DELTA(cubic(root2), 0.0, tol);
    TS_ASSERT_DELTA(cubic(root3), 0.0, tol);

    std::tie(a0, a1, a2, a3) = std::make_tuple(12.0, -4.0, -3.0, 1.0);
    roots_cubic(a0, a1, a2, a3, 1e-15, nroots, root1, root2, root3);

    TS_ASSERT_EQUALS(nroots, 3);
    TS_ASSERT_DELTA(root1, -2.0, tol);
    TS_ASSERT_DELTA(root2, 2.0, tol);
    TS_ASSERT_DELTA(root3, 3.0, tol);
    TS_ASSERT_DELTA(cubic(root1), 0.0, tol);
    TS_ASSERT_DELTA(cubic(root2), 0.0, tol);
    TS_ASSERT_DELTA(cubic(root3), 0.0, tol);

    std::tie(a0, a1, a2, a3) = std::make_tuple(0.0, 0.0, 0.0, 1.0);
    roots_cubic(a0, a1, a2, a3, 1e-15, nroots, root1, root2, root3);

    TS_ASSERT_EQUALS(nroots, 3);
    TS_ASSERT_DELTA(root1, 0.0, tol);
    TS_ASSERT_DELTA(root2, 0.0, tol);
    TS_ASSERT_DELTA(root3, 0.0, tol);
    TS_ASSERT_DELTA(cubic(root1), 0.0, tol);
    TS_ASSERT_DELTA(cubic(root2), 0.0, tol);
    TS_ASSERT_DELTA(cubic(root3), 0.0, tol);
  }

  void xtest_roots_quartic() {
    double root1 = 0.0, root2 = 0.0, root3 = 0.0, root4 = 0.0;
    int nroots = -1;
    const double tol = 1e-15;

    double a0, a1, a2, a3, a4;
    auto quartic = [&](double x){return a4*x*x*x*x + a3*x*x*x + a2*x*x + a1*x + a0;};

    std::tie(a0, a1, a2, a3, a4) = std::make_tuple(50.0, 10.0, -9.0, -2.0, 1.0);
    roots_quartic(a0, a1, a2, a3, a4, 1e-15, nroots, root1, root2, root3, root4);
    TS_ASSERT_EQUALS(nroots, 0);

    std::tie(a0, a1, a2, a3, a4) = std::make_tuple(45.0, 6.0, -10.0, -2.0, 1.0);
    roots_quartic(a0, a1, a2, a3, a4, 1e-15, nroots, root1, root2, root3, root4);
    TS_ASSERT_EQUALS(nroots, 2);
    TS_ASSERT_DELTA(root1, 3.0, tol);
    TS_ASSERT_DELTA(root2, 3.0, tol);
    TS_ASSERT_DELTA(quartic(root1), 0.0, tol);
    TS_ASSERT_DELTA(quartic(root2), 0.0, tol);

    std::tie(a0, a1, a2, a3, a4) = std::make_tuple(-45.0, -36.0, -4.0, 4.0, 1.0);
    roots_quartic(a0, a1, a2, a3, a4, 1e-15, nroots, root1, root2, root3, root4);
    TS_ASSERT_EQUALS(nroots, 2);
    TS_ASSERT_DELTA(root1, -3.0, tol);
    TS_ASSERT_DELTA(root2, 3.0, tol);
    TS_ASSERT_DELTA(quartic(root1), 0.0, tol);
    TS_ASSERT_DELTA(quartic(root2), 0.0, tol);

    std::tie(a0, a1, a2, a3, a4) = std::make_tuple(30.0, -1.0, -9.0, -1.0, 1.0);
    roots_quartic(a0, a1, a2, a3, a4, 1e-15, nroots, root1, root2, root3, root4);
    TS_ASSERT_EQUALS(nroots, 2);
    TS_ASSERT_DELTA(root1, 2.0, tol);
    TS_ASSERT_DELTA(root2, 3.0, tol);
    TS_ASSERT_DELTA(quartic(root1), 0.0, tol);
    TS_ASSERT_DELTA(quartic(root2), 0.0, tol);

    std::tie(a0, a1, a2, a3, a4) = std::make_tuple(24.0, 4.0, -10.0, -1.0, 1.0);
    roots_quartic(a0, a1, a2, a3, a4, 1e-15, nroots, root1, root2, root3, root4);
    TS_ASSERT_EQUALS(nroots, 4);
    TS_ASSERT_DELTA(root1, -2.0, tol);
    TS_ASSERT_DELTA(root2, -2.0, tol);
    TS_ASSERT_DELTA(root3, 2.0, tol);
    TS_ASSERT_DELTA(root4, 3.0, tol);
    TS_ASSERT_DELTA(quartic(root1), 0.0, tol);
    TS_ASSERT_DELTA(quartic(root2), 0.0, tol);
    TS_ASSERT_DELTA(quartic(root3), 0.0, tol);
    TS_ASSERT_DELTA(quartic(root4), 0.0, tol);

    std::tie(a0, a1, a2, a3, a4) = std::make_tuple(43.56, -3.3, -14.24, 0.5, 1.0);
    roots_quartic(a0, a1, a2, a3, a4, 1e-15, nroots, root1, root2, root3, root4);
    //std::cerr << nroots << ' ' << root1 << ' ' << root2 << ' ' << root3 << ' ' << root4 << std::endl;
    TS_ASSERT_EQUALS(nroots, 4);
    TS_ASSERT_DELTA(root1, -3.3, tol);
    TS_ASSERT_DELTA(root2, -2.2, tol);
    TS_ASSERT_DELTA(root3, 2.0, tol);
    TS_ASSERT_DELTA(root4, 3.0, tol);
    TS_ASSERT_DELTA(quartic(root1), 0.0, tol*100);
    TS_ASSERT_DELTA(quartic(root2), 0.0, tol*10);
    TS_ASSERT_DELTA(quartic(root3), 0.0, tol);
    TS_ASSERT_DELTA(quartic(root4), 0.0, tol*10);
  }

  void test_dtrs_solve_main_1d() {
    std::cerr <<"\n\nSolve main\n\n";
    dtrs_control_type control;
    int n = 1;
    DoubleFortranVector c(n);
    DoubleFortranVector h(n);
    DoubleFortranVector x(n);

    {
      dtrs_inform_type inform;
      double radius = 10.0;
      double f = 1;
      h(1) = 2.0;
      c(1) = 2.0;
      dtrs_solve_main(n, radius, f, c, h, x, control, inform);
      std::cerr << x << std::endl;
      TS_ASSERT_DELTA(x(1), -1, 1e-10);
    }
    {
      dtrs_inform_type inform;
      double radius = 10.0;
      double f = 0;
      h(1) = 2.0;
      c(1) = -2.0;
      dtrs_solve_main(n, radius, f, c, h, x, control, inform);
      std::cerr << x << std::endl;
      TS_ASSERT_DELTA(x(1), 1, 1e-10);
    }
    {
      dtrs_inform_type inform;
      double radius = 1.0;
      double f = 0;
      h(1) = 2.0;
      c(1) = -2.0;
      dtrs_solve_main(n, radius, f, c, h, x, control, inform);
      std::cerr << x << std::endl;
      TS_ASSERT_DELTA(x(1), 1, 1e-10);
    }
    {
      dtrs_inform_type inform;
      double radius = 0.5;
      double f = 1;
      h(1) = 2.0;
      c(1) = 2.0;
      dtrs_solve_main(n, radius, f, c, h, x, control, inform);
      std::cerr << x << std::endl;
      TS_ASSERT_DELTA(x(1), -0.5, 1e-10);
    }
    {
      dtrs_inform_type inform;
      double radius = 2.0;
      double f = 0;
      h(1) = -2.0;
      c(1) = -2.0;
      dtrs_solve_main(n, radius, f, c, h, x, control, inform);
      std::cerr << x << ' ' << inform.obj << std::endl;

      TS_ASSERT_DELTA(x(1), 1, 1e-10);
    }
  }
};

#endif // CURVEFITTING_RAL_NLLS_DTRSTEST_H_
