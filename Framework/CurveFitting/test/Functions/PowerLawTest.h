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
#include "MantidCurveFitting/Functions/PowerLaw.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"

// using namespace Mantid;
// using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::CurveFitting;
using namespace Mantid::CurveFitting::Functions;
using Mantid::MantidVec;

class PowerLawTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PowerLawTest *createSuite() { return new PowerLawTest(); }
  static void destroySuite(PowerLawTest *suite) { delete suite; }

  void test_category() {
    PowerLaw fn;
    TS_ASSERT_EQUALS(fn.category(), "General; Muon\\MuonModelling");
  }

  void test_function_gives_expected_value_for_given_input() {
    PowerLaw pl;
    pl.initialize();

    TS_ASSERT_THROWS(pl.setParameter("X", 1.0), const std::invalid_argument &);
    TS_ASSERT_THROWS(pl.setParameter("A9", 1.0), const std::invalid_argument &);

    const double magnitude = 2.3;
    const double exponent = 4.0;
    const double constant = 7.2;

    pl.setParameter("Magnitude", magnitude);
    pl.setParameter("Exponent", exponent);
    pl.setParameter("Constant", constant);

    const std::size_t numPoints = 100;
    std::array<double, numPoints> xValues;
    std::iota(xValues.begin(), xValues.end(), 0);
    std::array<double, numPoints> yValues;
    pl.function1D(yValues.data(), xValues.data(), numPoints);

    for (size_t i = 0; i < numPoints; i++) {
      TS_ASSERT_DELTA(yValues[i], constant + magnitude * pow(i, exponent), 1e-12);
    }
  }
};
