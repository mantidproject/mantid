#ifndef MANTID_KERNEL_DIFFRACTIONTEST_H_
#define MANTID_KERNEL_DIFFRACTIONTEST_H_

#include <cxxtest/TestSuite.h>
#include <limits>

#include "MantidKernel/Diffraction.h"

using Mantid::Kernel::Diffraction::calcTofMax;
using Mantid::Kernel::Diffraction::calcTofMin;

namespace {                // anonymous
const double DIFC = 2100.; // sensible value
const double TZERO = 10.;
// intentionally goofy - reduces tzero by 1
const double DIFA1 = .25 * DIFC * DIFC;
// intentionally goofy - reduces tzero by .01
const double DIFA2 = 25 * DIFC * DIFC;
// intentionally goofy
const double DIFA3 = -.25 * DIFC * DIFC;
} // namespace

class DiffractionTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static DiffractionTest *createSuite() { return new DiffractionTest(); }
  static void destroySuite(DiffractionTest *suite) { delete suite; }

  void test_calcTofMin() {
    const double TMIN = 300.;

    // just difc
    TS_ASSERT_EQUALS(calcTofMin(DIFC, 0., 0.), 0.);
    TS_ASSERT_EQUALS(calcTofMin(DIFC, 0., 0., TMIN), TMIN);
    // difc + tzero
    TS_ASSERT_EQUALS(calcTofMin(DIFC, 0., TZERO, 0.), TZERO);
    TS_ASSERT_EQUALS(calcTofMin(DIFC, 0., TZERO, TMIN), TMIN);

    // difc + difa + tzero
    TS_ASSERT_EQUALS(calcTofMin(DIFC, DIFA1, 0., 0.), 0.);
    TS_ASSERT_EQUALS(calcTofMin(DIFC, DIFA1, 0., TMIN), TMIN);
    TS_ASSERT_EQUALS(calcTofMin(DIFC, DIFA1, TZERO, 0.), TZERO - 1.);
    TS_ASSERT_EQUALS(calcTofMin(DIFC, DIFA1, TZERO, TMIN), TMIN);

    TS_ASSERT_EQUALS(calcTofMin(DIFC, DIFA2, 0., 0.), 0.);
    TS_ASSERT_EQUALS(calcTofMin(DIFC, DIFA2, 0., TMIN), TMIN);
    TS_ASSERT_EQUALS(calcTofMin(DIFC, DIFA2, TZERO, 0.), TZERO - .01);
    TS_ASSERT_EQUALS(calcTofMin(DIFC, DIFA2, TZERO, TMIN), TMIN);
  }

  void test_calcTofMax() {
    const double TMAX = 16666.7;
    const double TSUPERMAX = std::numeric_limits<double>::max();

    // just difc
    TS_ASSERT_EQUALS(calcTofMax(DIFC, 0., 0., TMAX), TMAX);
    TS_ASSERT_EQUALS(calcTofMax(DIFC, 0., 0., TSUPERMAX), TSUPERMAX);
    // difc + tzero
    TS_ASSERT_EQUALS(calcTofMax(DIFC, 0., TZERO, TMAX), TMAX);
    TS_ASSERT_EQUALS(calcTofMax(DIFC, 0., TZERO, TSUPERMAX), TSUPERMAX);

    // difc + difa + tzero
    TS_ASSERT_EQUALS(calcTofMax(DIFC, DIFA1, 0., TMAX), TMAX);
    TS_ASSERT_EQUALS(calcTofMax(DIFC, DIFA1, 0., TSUPERMAX), TSUPERMAX);
    TS_ASSERT_EQUALS(calcTofMax(DIFC, DIFA1, TZERO, TMAX), TMAX);
    TS_ASSERT_EQUALS(calcTofMax(DIFC, DIFA1, TZERO, TSUPERMAX), TSUPERMAX);

    TS_ASSERT_EQUALS(calcTofMax(DIFC, DIFA3, 0., TMAX), 0.);
    TS_ASSERT_EQUALS(calcTofMax(DIFC, DIFA3, 0., TSUPERMAX), 0.);
    TS_ASSERT_EQUALS(calcTofMax(DIFC, DIFA3, TZERO, TMAX), TZERO - 1.);
    TS_ASSERT_EQUALS(calcTofMax(DIFC, DIFA3, TZERO, TSUPERMAX), TZERO - 1.);
  }
};

#endif /* MANTID_KERNEL_DIFFRACTIONTEST_H_ */
