#ifndef MANTID_CURVEFITTING_BK2BKEXPCONVPVTEST_H_
#define MANTID_CURVEFITTING_BK2BKEXPCONVPVTEST_H_

#include <cxxtest/TestSuite.h>
#include <fstream>

#include "MantidCurveFitting/Functions/Bk2BkExpConvPV.h"

using namespace Mantid::CurveFitting::Functions;

class Bk2BkExpConvPVTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static Bk2BkExpConvPVTest *createSuite() { return new Bk2BkExpConvPVTest(); }
  static void destroySuite(Bk2BkExpConvPVTest *suite) { delete suite; }

  void test_cateogry() {
    Bk2BkExpConvPV fn;
    TS_ASSERT_EQUALS(fn.category(), "Peak");
  }

  void test_functionCalculator() {

    Bk2BkExpConvPV peak;
    peak.initialize();
    peak.setParameter("Height", 100.0);
    peak.setParameter("TOF_h", 400.0);
    peak.setParameter("Alpha", 1.0);
    peak.setParameter("Beta", 1.5);
    peak.setParameter("Sigma2", 200.0);
    peak.setParameter("Gamma", 0.0);

    Mantid::API::FunctionDomain1DVector x(300, 500, 100);
    Mantid::API::FunctionValues y(x);

    // We should get a peak at ~400
    TS_ASSERT_THROWS_NOTHING(peak.function(x, y));
    TS_ASSERT_DELTA(y[0], 0.0000, 1e-4);
    TS_ASSERT_DELTA(y[50], 2.7983, 1e-4);
    TS_ASSERT_DELTA(y[99], 0.0000, 1e-4);
  }
};

#endif /* MANTID_CURVEFITTING_BK2BKEXPCONVPVTEST_H_ */
