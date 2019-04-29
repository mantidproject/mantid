// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef IKEDACARPENTERPVTEST_H_
#define IKEDACARPENTERPVTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/Axis.h"
#include "MantidCurveFitting/Algorithms/Fit.h"
#include "MantidCurveFitting/Functions/IkedaCarpenterPV.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/ConfigService.h"

#include <boost/scoped_array.hpp>

using namespace Mantid::CurveFitting::Functions;

class IkedaCarpenterPVTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static IkedaCarpenterPVTest *createSuite() {
    return new IkedaCarpenterPVTest();
  }
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
    TS_ASSERT_DELTA(fn.intensity(), 810.7256, 1e-4);
  }
};

#endif /*IKEDACARPENTERPVTEST_H_*/
