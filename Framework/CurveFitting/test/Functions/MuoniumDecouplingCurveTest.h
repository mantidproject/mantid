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
#include "MantidCurveFitting/Functions/MuoniumDecouplingCurve.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"

using namespace Mantid::API;
using namespace Mantid::CurveFitting;
using namespace Mantid::CurveFitting::Functions;
using Mantid::MantidVec;

class MuoniumDecouplingCurveTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MuoniumDecouplingCurveTest *createSuite() { return new MuoniumDecouplingCurveTest(); }
  static void destroySuite(MuoniumDecouplingCurveTest *suite) { delete suite; }

  void test_category() {
    MuoniumDecouplingCurve fn;
    TS_ASSERT_EQUALS(fn.category(), "Muon\\MuonModelling");
  }

  void test_function_gives_expected_value_for_given_input() {
    MuoniumDecouplingCurve mdc;
    mdc.initialize();

    TS_ASSERT_THROWS(mdc.setParameter("X", 1.0), const std::invalid_argument &);
    TS_ASSERT_THROWS(mdc.setParameter("A9", 1.0), const std::invalid_argument &);

    const double repolarisingAsym = 1.3;
    const double decouplingField = 2.0;
    const double bkgdAsym = 5.2;

    mdc.setParameter("RepolarisingAsymmetry", repolarisingAsym);
    mdc.setParameter("DecouplingField", decouplingField);
    mdc.setParameter("BackgroundAsymmetry", bkgdAsym);

    const std::size_t numPoints = 100;
    std::array<double, numPoints> xValues;
    std::iota(xValues.begin(), xValues.end(), 0);
    std::array<double, numPoints> yValues;
    mdc.function1D(yValues.data(), xValues.data(), numPoints);

    for (size_t i = 0; i < numPoints; i++) {
      TS_ASSERT_DELTA(yValues[i],
                      repolarisingAsym * (0.5 + pow(xValues[i] / decouplingField, 2)) /
                              (1 + pow(xValues[i] / decouplingField, 2)) +
                          bkgdAsym,
                      1e-12);
    }
  }
};
