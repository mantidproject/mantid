#ifndef CURVEFITTING_RAL_NLLS_NLLSTEST_H_
#define CURVEFITTING_RAL_NLLS_NLLSTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/RalNlls/Internal.h"
#include "MantidCurveFitting/RalNlls/NLLS.h"
//#include "MantidCurveFitting/FuncMinimizers/MoreSorensenMinimizer.h"

#include <iostream>

using namespace Mantid::CurveFitting::RalNlls;
using namespace Mantid::CurveFitting;
//using namespace Mantid::CurveFitting::FuncMinimisers;

namespace {
  struct params_type {
    DoubleFortranVector t;
    DoubleFortranVector y;
  };

void eval_r(int status, int n, int m, const DoubleFortranVector &x, DoubleFortranVector &r, params_base_type params) {

  double x1, x2;

  x1 = x(1);
  x2 = x(2);
  auto aparams = reinterpret_cast<params_type*>(params);
  for(int i = 1; i <= m; ++i) {
    r(i) = x1 * exp(x2*aparams->t(i)) - aparams->y(i);
  }
  status = 0; // ! Success
}

void eval_J(int status, int n, int m, const DoubleFortranVector& x, DoubleFortranMatrix& J, params_base_type p) {

  double x1 = x(1);
  double x2 = x(2);
  auto params = reinterpret_cast<params_type*>(p);
  for(int i = 1; i <= m; ++i) {
    J(i, 1) = exp(x2*params->t(i));
    J(i, 2) = params->t(i) * x1 * exp(x2*params->t(i));
  }
  status = 0; // ! Success
}

void eval_HF(int status, int n, int m, const DoubleFortranVector& x, const DoubleFortranVector& r, DoubleFortranMatrix& HF, params_base_type p) {

  //double x1 = x(1);
  //double x2 = x(2);
  //auto params = reinterpret_cast<params_type*>(p);
  //double H12 = 0.0;
  //double H22 = 0.0;
  //  
  //for(int i = 1; i <= m; ++i) {
  //  H12 += r(i) * params->t(i) * exp(x2*params->t(i));
  //  H22 += r(i) * pow(params->t(i), 2) * exp(x2*params->t(i));
  //}
  //HF(1, 1) = 0.0;
  //HF(1, 2) = H12;
  //HF(2, 1) = H12;
  //HF(2, 2) = H22;
  status = 0; // ! Success
}

void eval_r_ExpDecay(int status, int n, int m, const DoubleFortranVector &x, DoubleFortranVector &r, params_base_type p) {
  double x1 = x(1);
  double x2 = x(2);
  auto params = reinterpret_cast<params_type*>(p);
  for(int i = 1; i <= m; ++i) {
    r(i) = x1 * exp(-params->t(i)/x2) - params->y(i);
  }
  status = 0; // ! Success
}

void eval_J_ExpDecay(int status, int n, int m, const DoubleFortranVector& x, DoubleFortranMatrix& J, params_base_type p) {
  double x1 = x(1);
  double x2 = x(2);
  auto params = reinterpret_cast<params_type*>(p);
  for(int i = 1; i <= m; ++i) {
    J(i, 1) = exp(-params->t(i)/x2);
    J(i, 2) =  x1 * J(i, 1) * params->t(i) / pow(x2, 2); // x1 * e * t / x2 / x2
  }
  status = 0; // ! Success
}
} // namespace


class NLLSTest : public CxxTest::TestSuite {
public:

  void xtest_it_works() {
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

    options.nlls_method = 3;
    nlls_solve(n, m, x, eval_r, eval_J, eval_HF, &params, options, inform, weights);

    std::cerr << std::endl << x(1) << ' ' << x(2) << std::endl;

  }

  void test_MoreSorensen() {
    //MoreSorensenMinimizer minimizer;
    std::vector<double> y({62,48,51,36,35,22,23,17,22,10,12,12,14,12,10,9,3,6,3,4,5,4,2,3,2,2,0,2,0,2,1,0,1,1,0,0,1,0,1,0,0,0,1,1,0,0,0,0});
    std::vector<double> e({7.87400787401181,6.92820323027551,7.14142842854285,6,5.91607978309962,4.69041575982343,4.79583152331272,4.12310562561766,4.69041575982343,3.16227766016838,3.46410161513775,3.46410161513775,3.74165738677394,3.46410161513775,3.16227766016838,3,1.73205080756888,2.44948974278318,1.73205080756888,2,2.23606797749979,2,1.4142135623731,1.73205080756888,1.4142135623731,1.4142135623731,0,1.4142135623731,0,1.4142135623731,1,0,1,1,0,0,1,0,1,0,0,0,1,1,0,0,0,0});
    std::vector<double> t({0.0900000035762787,0.409999996423721,0.730000019073486,1.05000007152557,1.37000000476837,1.68999993801117,2.01000022888184,2.33000016212463,2.65000009536743,2.97000026702881,3.29000020027161,3.61000037193298,3.9300000667572,4.25,4.56999969482422,4.8899998664856,5.21000003814697,5.52999973297119,5.84999990463257,6.17000007629395,6.48999977111816,6.80999994277954,7.13000011444092,7.45000028610229,7.76999998092651,8.08999919891357,8.40999984741211,8.72999954223633,9.04999923706055,9.36999988555908,9.6899995803833,10.0099992752075,10.3299989700317,10.6499996185303,10.9699993133545,11.2899990081787,11.6099996566772,11.9299993515015,12.2499990463257,12.5699996948242,12.8899993896484,13.2099990844727,13.5299997329712,13.8499994277954,14.1699991226196,14.4899997711182,14.8099994659424,15.1299991607666});
    
    nlls_options options;
    nlls_inform inform;
    params_type params;
    int m = (int)y.size();
    int n = 2;
    params.t.allocate(m);
    params.y.allocate(m);

    DoubleFortranVector weights(m);
    for(int i = 0; i < m; ++i) {
      params.t(i+1) = t[i];
      params.y(i+1) = y[i];
      weights (i+1) = e[i] != 0.0 ? 1.0 / e[i] : 1.0;
    }

    DoubleFortranVector x(2);
    x(1) = 1.0;
    x(2) = 1.0;

    options.nlls_method = 3;
    nlls_solve(n, m, x, eval_r_ExpDecay, eval_J_ExpDecay, eval_HF, &params, options, inform, weights);

    std::cerr << std::endl << x(1) << ' ' << x(2) << std::endl;
  }

};

#endif // CURVEFITTING_RAL_NLLS_NLLSTEST_H_
