// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef CHEBYSHEVTEST_H_
#define CHEBYSHEVTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidCurveFitting/Functions/Chebyshev.h"

#include <array>

using namespace Mantid::API;
using Mantid::CurveFitting::Functions::Chebyshev;

class ChebyshevTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ChebyshevTest *createSuite() { return new ChebyshevTest(); }
  static void destroySuite(ChebyshevTest *suite) { delete suite; }

  void test_category() {
    Chebyshev cfn;
    cfn.initialize();

    std::vector<std::string> cats;
    TS_ASSERT_THROWS_NOTHING(cats = cfn.categories());
    TS_ASSERT_LESS_THAN_EQUALS(1, cats.size());
    TS_ASSERT_EQUALS(cats.front(), "Background");
    // This would enfonce one and only one category:
    // TS_ASSERT(cfn.category() == "Background");
  }

  void testNegative() {
    Chebyshev cheb;
    cheb.initialize();

    TS_ASSERT_THROWS(cheb.setAttributeValue("A0", 3.3), const std::invalid_argument &);
    TS_ASSERT_THROWS(cheb.setAttributeValue("n", -1), const std::invalid_argument &);
  }

  void testZero() {
    Chebyshev cheb;
    cheb.initialize();

    TS_ASSERT_THROWS(cheb.setAttributeValue("A1", 3.3), const std::invalid_argument &);
    TS_ASSERT_THROWS_NOTHING(cheb.setAttributeValue("n", 0));
  }

  void test_wrongStartEnd() {
    Chebyshev cheb;
    cheb.initialize();
    TS_ASSERT_THROWS(cheb.getAttribute("AX"), const std::invalid_argument &);
    TS_ASSERT_EQUALS(cheb.getAttribute("StartX").asDouble(), -1.0);
    TS_ASSERT_EQUALS(cheb.getAttribute("EndX").asDouble(), 1.0);

    double startx = 10.0;
    double endx = -10.0;
    cheb.setAttributeValue("StartX", startx);
    cheb.setAttributeValue("EndX", endx);

    TS_ASSERT_EQUALS(cheb.getAttribute("StartX").asDouble(), startx);
    TS_ASSERT_EQUALS(cheb.getAttribute("EndX").asDouble(), endx);

    FunctionDomain1DVector x(startx, endx, 10);
    FunctionValues y(x);

    TS_ASSERT_THROWS(cheb.function(x, y), const std::runtime_error &);

    startx = 10.0;
    endx = startx;

    cheb.setAttributeValue("StartX", startx);
    cheb.setAttributeValue("EndX", endx);

    TS_ASSERT_EQUALS(cheb.getAttribute("StartX").asDouble(), startx);
    TS_ASSERT_EQUALS(cheb.getAttribute("EndX").asDouble(), endx);

    FunctionDomain1DVector x1(startx, endx, 100);
    FunctionValues y1(x1);

    TS_ASSERT_THROWS(cheb.function(x1, y1), const std::runtime_error &);
  }

  void testValuesWorkspace() {
    const int N = 3;
    Chebyshev cheb;
    cheb.setAttributeValue("n", N);
    cheb.setAttributeValue("StartX", -10.0);
    cheb.setAttributeValue("EndX", 10.0);

    Mantid::API::MatrixWorkspace_sptr ws =
        WorkspaceFactory::Instance().create("Workspace2D", 1, 21, 21);
    Mantid::MantidVec &X = ws->dataX(0);

    Mantid::API::FunctionDomain1DVector domain(X);
    Mantid::API::FunctionValues values(domain);

    cheb.function(domain, values);
    for (size_t i = 0; i < domain.size(); ++i) {
      TS_ASSERT_DELTA(values[i], cos(N * acos(values[i])), 1e-12);
    }
  }

  void testValues() {

    const int N = 11;
    std::array<double, N> y, x;
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

  void test_change_n() {
    Chebyshev cheb;
    cheb.setAttributeValue("n", 3);
    cheb.setParameter("A0", 4.0);
    cheb.setParameter("A1", 3.0);
    cheb.setParameter("A2", 2.0);
    cheb.setParameter("A3", 1.0);
    cheb.setAttributeValue("n", 5);
    TS_ASSERT_EQUALS(cheb.getParameter(0), 4.0);
    TS_ASSERT_EQUALS(cheb.getParameter(1), 3.0);
    TS_ASSERT_EQUALS(cheb.getParameter(2), 2.0);
    TS_ASSERT_EQUALS(cheb.getParameter(3), 1.0);
    TS_ASSERT_EQUALS(cheb.getParameter(4), 0.0);
    TS_ASSERT_EQUALS(cheb.getParameter(5), 0.0);
  }

  void test_change_n_1() {
    Chebyshev cheb;
    cheb.setAttributeValue("n", 5);
    cheb.setParameter("A0", 4.0);
    cheb.setParameter("A1", 3.0);
    cheb.setParameter("A2", 2.0);
    cheb.setParameter("A3", 1.0);
    cheb.setParameter("A4", -1.0);
    cheb.setParameter("A5", -2.0);
    cheb.setAttributeValue("n", 3);
    TS_ASSERT_EQUALS(cheb.getParameter(0), 4.0);
    TS_ASSERT_EQUALS(cheb.getParameter(1), 3.0);
    TS_ASSERT_EQUALS(cheb.getParameter(2), 2.0);
    TS_ASSERT_EQUALS(cheb.getParameter(3), 1.0);
  }
};

#endif /*CHEBYSHEVTEST_H_*/
