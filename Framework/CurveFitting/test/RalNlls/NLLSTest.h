#ifndef CURVEFITTING_RAL_NLLS_NLLSTEST_H_
#define CURVEFITTING_RAL_NLLS_NLLSTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/RalNlls/Internal.h"
#include "MantidCurveFitting/RalNlls/NLLS.h"

#include <iostream>

using namespace Mantid::CurveFitting::RalNlls;
using namespace Mantid::CurveFitting;

namespace {
  struct params_type {
    DoubleFortranVector t;
    DoubleFortranVector y;
  };

void eval_r(int status, int n, int m, const DoubleFortranVector &x, DoubleFortranVector &r, params_base_type params) {

  double x1, x2;

  std::cerr << "fun" << std::endl;
  x1 = x(1);
  x2 = x(2);
  auto aparams = reinterpret_cast<params_type*>(params);
  for(int i = 1; i <= m; ++i) {
    r(i) = x1 * exp(x2*aparams->t(i)) - aparams->y(i);
  }
  status = 0; // ! Success
}

void eval_J(int status, int n, int m, const DoubleFortranVector& x, DoubleFortranMatrix& J, params_base_type p) {

  std::cerr << "grad" << std::endl;
  double x1 = x(1);
  double x2 = x(2);
  auto params = reinterpret_cast<params_type*>(p);
  for(int i = 1; i <= m; ++i) {
    J(i, 1) = exp(x2*params->t(i));
    J(i, 2) = params->t(i) * x1 * exp(x2*params->t(i));
    std::cerr << "J(" << i << ",1)=" << J(i,1) << "    J(" << i << ",2)=" << J(i,2) << std::endl;
  }
  status = 0; // ! Success
}

void eval_HF(int status, int n, int m, const DoubleFortranVector& x, const DoubleFortranVector& r, DoubleFortranMatrix& HF, params_base_type p) {

  std::cerr << "Hess" << std::endl;
  double x1 = x(1);
  double x2 = x(2);
  auto params = reinterpret_cast<params_type*>(p);
  double H12 = 0.0;
  double H22 = 0.0;
    
  for(int i = 1; i <= m; ++i) {
    H12 += r(i) * params->t(i) * exp(x2*params->t(i));
    H22 += r(i) * pow(params->t(i), 2) * exp(x2*params->t(i));
  }
  HF(1, 1) = 0.0;
  HF(1, 2) = H12;
  HF(2, 1) = H12;
  HF(2, 2) = H22;
  status = 0; // ! Success
}

} // namespace


class NLLSTest : public CxxTest::TestSuite {
public:

  void test_it_works() {
    nlls_options options;
    nlls_inform inform;
    params_type params;
    int m = 5;
    int n = 2;
    params.t.allocate(m);
    params.y.allocate(m);

    params.t(1) = 1.0;
    params.t(2) = 2.0;
    params.t(3) = 4.0;
    params.t(4) = 5.0;
    params.t(5) = 8.0;

    params.y(1) = 3.0;
    params.y(2) = 4.0;
    params.y(3) = 6.0;
    params.y(4) = 11.0;
    params.y(5) = 20.0;

    DoubleFortranVector x(2);
    x(1) = 2.5;
    x(2) = 0.25;

    DoubleFortranVector weights(m);
    weights(1) = 1.0;
    weights(2) = 1.0;
    weights(3) = 1.0;
    weights(4) = 1.0;
    weights(5) = 1.0;

    nlls_solve(n, m, x, eval_r, eval_J, eval_HF, &params, options, inform, weights);

    std::cerr << x(1) << ' ' << x(2) << std::endl;

  }

};

#endif // CURVEFITTING_RAL_NLLS_NLLSTEST_H_
