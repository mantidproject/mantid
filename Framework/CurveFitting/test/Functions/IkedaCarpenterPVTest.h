// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/Axis.h"
#include "MantidAPI/FunctionDomain1D.h"
#include "MantidAPI/FunctionValues.h"
#include "MantidCurveFitting/Algorithms/Fit.h"
#include "MantidCurveFitting/Functions/IkedaCarpenterPV.h"
#include "MantidCurveFitting/Jacobian.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/ConfigService.h"

#include <boost/scoped_array.hpp>
#include <cmath>

using namespace Mantid::CurveFitting::Functions;

class IkedaCarpenterPVTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static IkedaCarpenterPVTest *createSuite() { return new IkedaCarpenterPVTest(); }
  static void destroySuite(IkedaCarpenterPVTest *suite) { delete suite; }

  void test_category() {

    IkedaCarpenterPV fn;
    TS_ASSERT(fn.categories().size() == 1);
    TS_ASSERT(fn.category() == "Peak");
  }

  void test_values() {

    IkedaCarpenterPV fn;
    fn.initialize();

    // The following parameter values are results from a fit
    fn.setParameter("I", 3101.672);
    fn.setParameter("Alpha0", 1.6);
    fn.setParameter("Alpha1", 1.5);
    fn.setParameter("Beta0", 31.9);
    fn.setParameter("Kappa", 46.0);
    fn.setParameter("SigmaSquared", 99.935);
    fn.setParameter("Gamma", 0.0);
    fn.setParameter("X0", 49.984);

    // 1D domain
    Mantid::API::FunctionDomain1DVector x(0, 155, 31);
    Mantid::API::FunctionValues y(x);

    TS_ASSERT_THROWS_NOTHING(fn.function(x, y));
    TS_ASSERT_DELTA(y[9], 51.1755, 1e-4);
    TS_ASSERT_DELTA(y[10], 78.1676, 1e-4);
    TS_ASSERT_DELTA(y[11], 95.6899, 1e-4);
    TS_ASSERT_DELTA(y[12], 94.9801, 1e-4);
    TS_ASSERT_DELTA(y[13], 77.7493, 1e-4);
    TS_ASSERT_DELTA(y[14], 53.8871, 1e-4);
  }

  void test_intensity() {
    IkedaCarpenterPV fn;
    fn.initialize();
    fn.setParameter("I", 811.0867);
    fn.setParameter("Alpha0", 1.6);
    fn.setParameter("Alpha1", 1.5);
    fn.setParameter("Beta0", 31.9);
    fn.setParameter("Kappa", 46.0);
    fn.setParameter("SigmaSquared", 0.00281776);
    fn.setParameter("Gamma", 0.125);
    fn.setParameter("X0", 0);
    TS_ASSERT_DELTA(fn.intensity(), 810.7769, 1e-4);
  }

  void test_analytical_derivative_matches_numerical() {
    // Compare the analytical Jacobian (functionDeriv) against the numerical
    // Jacobian (calNumericalDeriv) for a representative parameter vector.
    // Without a workspace, wavelengths default to 1.0 for all data points,
    // which is sufficient to exercise the full derivative code path.

    IkedaCarpenterPV fn;
    fn.initialize();
    fn.setParameter("I", 1);
    fn.setParameter("Alpha0", 0.0001756);
    fn.setParameter("Alpha1", 5.125e-5);
    fn.setParameter("Beta0", 0.0022);
    fn.setParameter("Kappa", 19.3);
    fn.setParameter("SigmaSquared", 3.54e-6);
    fn.setParameter("Gamma", 0.00022);
    fn.setParameter("X0", 1.91);

    Mantid::API::IFunction1D &base = fn;

    const size_t nData = 31;
    const size_t nParams = fn.nParams(); // 8
    Mantid::API::FunctionDomain1DVector domain(0.0, 155.0, nData);

    Mantid::CurveFitting::Jacobian analyticalJac(nData, nParams);
    Mantid::CurveFitting::Jacobian numericalJac(nData, nParams);

    // Compute both Jacobians
    base.functionDeriv(domain, analyticalJac);
    fn.calNumericalDeriv(domain, numericalJac);

    // Compare element by element. Use a relative tolerance where the values
    // are large enough, and an absolute tolerance near zero.
    const double relTol = 5e-3; // 0.5% relative
    const double absTol = 1e-6; // absolute floor for near-zero entries
    size_t failCount = 0;

    for (size_t i = 0; i < nData; ++i) {
      for (size_t j = 0; j < nParams; ++j) {
        const double a = analyticalJac.get(i, j);
        const double n = numericalJac.get(i, j);
        const double scale = std::max({std::abs(a), std::abs(n), absTol});
        const double relDiff = std::abs(a - n) / scale;

        if (relDiff > relTol) {
          ++failCount;
          // Report first few failures for diagnostics
          if (failCount <= 5) {
            std::ostringstream msg;
            msg << "Derivative mismatch at data point " << i << ", param " << j << ": analytical=" << a
                << ", numerical=" << n << ", relDiff=" << relDiff;
            TS_FAIL(msg.str());
          }
        }
      }
    }
    TS_ASSERT_EQUALS(failCount, 0);
  }
};
