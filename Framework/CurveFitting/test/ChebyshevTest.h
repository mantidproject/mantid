#ifndef CHEBYSHEVTEST_H_
#define CHEBYSHEVTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/Chebyshev.h"
#include "MantidCurveFitting/Fit.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FunctionDomain1D.h"
#include "MantidAPI/FunctionValues.h"

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::CurveFitting;

class ChebyshevTest : public CxxTest::TestSuite {
public:
  void testValues() {
    const int N = 11;
    double y[N], x[N];
    for (int i = 0; i < N; ++i) {
      x[i] = i * 0.1;
    }
    Chebyshev cheb;
    cheb.setAttributeValue("n", 10);
    for (int n = 0; n <= 10; ++n) {
      cheb.setParameter(n, 1.);
      if (n > 0) {
        cheb.setParameter(n - 1, 0.);
      }
      cheb.function1D(&y[0], &x[0], N);
      for (int i = 0; i < N; ++i) {
        TS_ASSERT_DELTA(y[i], cos(n * acos(x[i])), 1e-12);
      }
    }
  }

  /** A test for [-1, 1] range data
    */
  void testFit() {
    Mantid::API::MatrixWorkspace_sptr ws =
        WorkspaceFactory::Instance().create("Workspace2D", 1, 11, 11);

    Mantid::MantidVec &X = ws->dataX(0);
    Mantid::MantidVec &Y = ws->dataY(0);
    Mantid::MantidVec &E = ws->dataE(0);
    for (size_t i = 0; i < Y.size(); i++) {
      double x = -1. + 0.1 * static_cast<double>(i);
      X[i] = x;
      Y[i] = x * x * x;
      E[i] = 1;
    }
    // X.back() = 1.;

    AnalysisDataService::Instance().addOrReplace("ChebyshevTest_ws", ws);

    Chebyshev cheb;
    cheb.setAttributeValue("n", 3);

    Fit fit;
    fit.initialize();

    fit.setPropertyValue("Function", cheb.asString());
    fit.setPropertyValue("InputWorkspace", "ChebyshevTest_ws");
    fit.setPropertyValue("WorkspaceIndex", "0");

    fit.execute();
    IFunction::Attribute StartX = cheb.getAttribute("StartX");
    TS_ASSERT_EQUALS(StartX.asDouble(), -1);
    IFunction::Attribute EndX = cheb.getAttribute("EndX");
    TS_ASSERT_EQUALS(EndX.asDouble(), 1);
    TS_ASSERT(fit.isExecuted());

    IFunction_sptr out = fit.getProperty("Function");
    TS_ASSERT_DELTA(out->getParameter("A0"), 0, 1e-12);
    TS_ASSERT_DELTA(out->getParameter("A1"), 0.75, 1e-12);
    TS_ASSERT_DELTA(out->getParameter("A2"), 0, 1e-12);
    TS_ASSERT_DELTA(out->getParameter("A3"), 0.25, 1e-12);

    // check its categories
    const std::vector<std::string> categories = out->categories();
    TS_ASSERT(categories.size() == 1);
    TS_ASSERT(categories[0] == "Background");

    AnalysisDataService::Instance().remove("ChebyshevTest_ws");
  }

  /** A test for a random number data
    */
  void testBackground() {
    Mantid::API::MatrixWorkspace_sptr ws =
        WorkspaceFactory::Instance().create("Workspace2D", 1, 21, 21);

    Mantid::MantidVec &X = ws->dataX(0);
    Mantid::MantidVec &Y = ws->dataY(0);
    Mantid::MantidVec &E = ws->dataE(0);
    for (size_t i = 0; i < Y.size(); i++) {
      double x = -10. + 1 * static_cast<double>(i);
      X[i] = x;
      Y[i] = x * x * x;
      if (Y[i] < 1.0) {
        E[i] = 1.0;
      } else {
        E[i] = sqrt(Y[i]);
      }

      // std::cout << X[i] << "   " << Y[i] << std::endl;
    }

    AnalysisDataService::Instance().add("ChebyshevTest_ws", ws);

    Chebyshev cheb;
    cheb.setAttributeValue("n", 3);
    cheb.setAttributeValue("StartX", -10.0);
    cheb.setAttributeValue("EndX", 10.0);

    Fit fit;
    fit.initialize();

    fit.setPropertyValue("Function", cheb.asString());
    fit.setPropertyValue("InputWorkspace", "ChebyshevTest_ws");
    fit.setPropertyValue("WorkspaceIndex", "0");

    fit.setProperty("Minimizer", "Levenberg-MarquardtMD");
    fit.setProperty("CostFunction", "Least squares");
    fit.setProperty("MaxIterations", 1000);

    fit.execute();
    IFunction::Attribute StartX = cheb.getAttribute("StartX");
    TS_ASSERT_EQUALS(StartX.asDouble(), -10.0);
    IFunction::Attribute EndX = cheb.getAttribute("EndX");
    TS_ASSERT_EQUALS(EndX.asDouble(), 10.0);
    TS_ASSERT(fit.isExecuted());

    double chi2 = fit.getProperty("OutputChi2overDoF");
    TS_ASSERT(chi2 < 2.0);

    IFunction_sptr out = fit.getProperty("Function");

    // check its categories
    const std::vector<std::string> categories = out->categories();
    TS_ASSERT(categories.size() == 1);
    TS_ASSERT(categories[0] == "Background");

    API::FunctionDomain1DVector domain(X);
    API::FunctionValues values(domain);

    out->function(domain, values);

    for (size_t i = 0; i < domain.size(); ++i) {
      TS_ASSERT_DELTA(values[i], Y[i], 0.1);
    }
  }
};

#endif /*CHEBYSHEVTEST_H_*/
