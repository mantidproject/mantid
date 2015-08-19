#ifndef MANTID_KERNEL_CHEBYSHEVSERIESTEST_H_
#define MANTID_KERNEL_CHEBYSHEVSERIESTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidKernel/Math/Distributions/ChebyshevSeries.h"
#include <boost/assign/list_of.hpp>

using Mantid::Kernel::ChebyshevSeries;

class ChebyshevSeriesTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ChebyshevSeriesTest *createSuite() {
    return new ChebyshevSeriesTest();
  }
  static void destroySuite(ChebyshevSeriesTest *suite) { delete suite; }

  void test_expected_values_for_x_in_range() {
    using namespace boost::assign;
    const double delta(1e-12);
    std::vector<double> coeffs = list_of(0.5)(2.4)(-3.2);

    const double x(0.75);
    // Expected values computed on paper using reccurrence relation
    // https://en.wikipedia.org/wiki/Clenshaw_algorithm#Special_case_for_Chebyshev_series
    TS_ASSERT_DELTA(0.5*coeffs[0], ChebyshevSeries(0)(coeffs, x), delta);
    TS_ASSERT_DELTA(2.05, ChebyshevSeries(1)(coeffs, x), delta);
    TS_ASSERT_DELTA(1.65, ChebyshevSeries(2)(coeffs, x), delta);
  }
};

#endif /* MANTID_KERNEL_CHEBYSHEVSERIESTEST_H_ */
