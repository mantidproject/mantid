#ifndef MANTID_SINQ_MILLERINDICESIOTEST_H
#define MANTID_SINQ_MILLERINDICESIOTEST_H

#include <cxxtest/TestSuite.h>
#include "MantidSINQ/PoldiUtilities/MillerIndicesIO.h"
#include <stdexcept>
#include "boost/lexical_cast.hpp"

using namespace Mantid::Poldi;

class MillerIndicesIOTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MillerIndicesIOTest *createSuite() {
    return new MillerIndicesIOTest();
  }
  static void destroySuite(MillerIndicesIOTest *suite) { delete suite; }

  void testToString() {
    MillerIndices hkl(1, 5, 3);

    TS_ASSERT_EQUALS(MillerIndicesIO::toString(hkl), "1 5 3");
  }

  void testFromString() {
    std::string hklString("1 4 5");

    MillerIndices hkl = MillerIndicesIO::fromString(hklString);
    TS_ASSERT_EQUALS(hkl.h(), 1);
    TS_ASSERT_EQUALS(hkl.k(), 4);
    TS_ASSERT_EQUALS(hkl.l(), 5);

    std::string hklInvalidStringLong("1 3 4 2");
    TS_ASSERT_THROWS(MillerIndicesIO::fromString(hklInvalidStringLong),
                     std::runtime_error);

    std::string hklInvalidStringShort("2 3");
    TS_ASSERT_THROWS(MillerIndicesIO::fromString(hklInvalidStringShort),
                     std::runtime_error);

    std::string hklInvalidStringGarbage("q43tn rufninc");
    TS_ASSERT_THROWS(MillerIndicesIO::fromString(hklInvalidStringGarbage),
                     boost::bad_lexical_cast);
  }

  void testComplementarity() {
    MillerIndices hkl(1, 2, 3);
    MillerIndices hklCopy =
        MillerIndicesIO::fromString(MillerIndicesIO::toString(hkl));

    TS_ASSERT_EQUALS(hkl.h(), hklCopy.h());
    TS_ASSERT_EQUALS(hkl.k(), hklCopy.k());
    TS_ASSERT_EQUALS(hkl.l(), hklCopy.l());

    std::string hklString("1 3 4");
    TS_ASSERT_EQUALS(
        MillerIndicesIO::toString(MillerIndicesIO::fromString(hklString)),
        "1 3 4");
  }
};

#endif // MANTID_SINQ_MILLERINDICESIOTEST_H
