// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef CURVEFITTING_RAL_NLLS_NLLSTEST_H_
#define CURVEFITTING_RAL_NLLS_NLLSTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AlgorithmFactory.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidCurveFitting/Algorithms/Fit.h"
#include "MantidCurveFitting/Functions/ExpDecay.h"

using namespace Mantid::CurveFitting;
using namespace Mantid::CurveFitting::Algorithms;
using namespace Mantid::API;

class NLLSTest : public CxxTest::TestSuite {
public:
  void test_Galahad_ExpDecay() {
    auto ws = make_exp_decay_workspace();
    Fit fit;
    fit.initialize();
    fit.setPropertyValue("Function", "name=ExpDecay");
    fit.setProperty("InputWorkspace", ws);
    fit.setProperty("Minimizer", "Trust Region");
    fit.execute();
    IFunction_sptr fun = fit.getProperty("Function");
    TS_ASSERT_DELTA(fun->getParameter(0), 60.195, 0.001);
    TS_ASSERT_DELTA(fun->getParameter(1), 2.16815, 0.00001);
  }

  void test_no_NaNs() {
    std::vector<double> x{1, 2, 3, 5, 7, 10};
    std::vector<double> y{109, 149, 149, 191, 213, 224};
    auto alg = AlgorithmFactory::Instance().create("CreateWorkspace", -1);
    alg->initialize();
    alg->setProperty("DataX", x);
    alg->setProperty("DataY", y);
    alg->setProperty("OutputWorkspace", "out");
    alg->execute();
    auto ws = AnalysisDataService::Instance().retrieveWS<Workspace>("out");
    AnalysisDataService::Instance().remove("out");
    Fit fit;
    fit.initialize();
    fit.setPropertyValue(
        "Function", "name=UserFunction,Formula=b1*(1-exp(-b2*x)),b1=1,b2=1");
    fit.setProperty("InputWorkspace", ws);
    fit.setProperty("Minimizer", "Trust Region");
    fit.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(fit.execute());
  }

private:
  Workspace_sptr make_exp_decay_workspace() {
    std::vector<double> y({62, 48, 51, 36, 35, 22, 23, 17, 22, 10, 12, 12,
                           14, 12, 10, 9,  3,  6,  3,  4,  5,  4,  2,  3,
                           2,  2,  0,  2,  0,  2,  1,  0,  1,  1,  0,  0,
                           1,  0,  1,  0,  0,  0,  1,  1,  0,  0,  0,  0});
    std::vector<double> e({7.87400787401181,
                           6.92820323027551,
                           7.14142842854285,
                           6,
                           5.91607978309962,
                           4.69041575982343,
                           4.79583152331272,
                           4.12310562561766,
                           4.69041575982343,
                           3.16227766016838,
                           3.46410161513775,
                           3.46410161513775,
                           3.74165738677394,
                           3.46410161513775,
                           3.16227766016838,
                           3,
                           1.73205080756888,
                           2.44948974278318,
                           1.73205080756888,
                           2,
                           2.23606797749979,
                           2,
                           1.4142135623731,
                           1.73205080756888,
                           1.4142135623731,
                           1.4142135623731,
                           0,
                           1.4142135623731,
                           0,
                           1.4142135623731,
                           1,
                           0,
                           1,
                           1,
                           0,
                           0,
                           1,
                           0,
                           1,
                           0,
                           0,
                           0,
                           1,
                           1,
                           0,
                           0,
                           0,
                           0});
    std::vector<double> t({0.0900000035762787, 0.409999996423721,
                           0.730000019073486,  1.05000007152557,
                           1.37000000476837,   1.68999993801117,
                           2.01000022888184,   2.33000016212463,
                           2.65000009536743,   2.97000026702881,
                           3.29000020027161,   3.61000037193298,
                           3.9300000667572,    4.25,
                           4.56999969482422,   4.8899998664856,
                           5.21000003814697,   5.52999973297119,
                           5.84999990463257,   6.17000007629395,
                           6.48999977111816,   6.80999994277954,
                           7.13000011444092,   7.45000028610229,
                           7.76999998092651,   8.08999919891357,
                           8.40999984741211,   8.72999954223633,
                           9.04999923706055,   9.36999988555908,
                           9.6899995803833,    10.0099992752075,
                           10.3299989700317,   10.6499996185303,
                           10.9699993133545,   11.2899990081787,
                           11.6099996566772,   11.9299993515015,
                           12.2499990463257,   12.5699996948242,
                           12.8899993896484,   13.2099990844727,
                           13.5299997329712,   13.8499994277954,
                           14.1699991226196,   14.4899997711182,
                           14.8099994659424,   15.1299991607666});
    auto alg = AlgorithmFactory::Instance().create("CreateWorkspace", -1);
    alg->initialize();
    alg->setProperty("DataX", t);
    alg->setProperty("DataY", y);
    alg->setProperty("DataE", e);
    alg->setProperty("OutputWorkspace", "exp_decay_workspace");
    alg->execute();
    auto ws = AnalysisDataService::Instance().retrieveWS<Workspace>(
        "exp_decay_workspace");
    AnalysisDataService::Instance().remove("exp_decay_workspace");
    return ws;
  }
};

#endif // CURVEFITTING_RAL_NLLS_NLLSTEST_H_
