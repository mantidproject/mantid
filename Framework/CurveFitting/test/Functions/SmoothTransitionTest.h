// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidCurveFitting/Functions/SmoothTransition.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

// using namespace Mantid;
// using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::CurveFitting;
using namespace Mantid::CurveFitting::Functions;
using Mantid::MantidVec;

class SmoothTransitionTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SmoothTransitionTest *createSuite() { return new SmoothTransitionTest(); }
  static void destroySuite(SmoothTransitionTest *suite) { delete suite; }

  void test_category() {
    SmoothTransition fn;
    TS_ASSERT_EQUALS(fn.category(), "Muon\\MuonModelling");
  }

  void test_function_gives_expected_value_for_given_input() {
    SmoothTransition st;
    st.initialize();

    TS_ASSERT_THROWS(st.setParameter("mid", 1.0), const std::invalid_argument &);
    TS_ASSERT_THROWS(st.setParameter("A9", 1.0), const std::invalid_argument &);

    const double a1 = 2.3;
    const double a2 = 4.0;
    const double midpoint = 7.2;
    const double gr = 1.0;

    st.setParameter("A1", a1);
    st.setParameter("A2", a2);
    st.setParameter("Midpoint", midpoint);
    st.setParameter("GrowthRate", gr);

    const std::size_t numPoints = 100;
    std::array<double, numPoints> xValues;
    std::iota(xValues.begin(), xValues.end(), 0);
    std::array<double, numPoints> yValues;
    st.function1D(yValues.data(), xValues.data(), numPoints);

    for (size_t i = 0; i < numPoints; i++) {
      TS_ASSERT_DELTA(yValues[i], a2 + (a1 - a2) / (exp(-(xValues[i] - midpoint) / gr) + 1), 1e-12);
    }
  }
};
