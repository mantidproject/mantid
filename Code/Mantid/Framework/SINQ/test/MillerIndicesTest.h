#ifndef MANTID_SINQ_MILLERINDICESTEST_H
#define MANTID_SINQ_MILLERINDICESTEST_H

#include <cxxtest/TestSuite.h>
#include "MantidSINQ/PoldiUtilities/MillerIndices.h"
#include <stdexcept>

using namespace Mantid::Poldi;

class MillerIndicesTest : public CxxTest::TestSuite
{
public:
    // This pair of boilerplate methods prevent the suite being created statically
    // This means the constructor isn't called when running other tests
    static MillerIndicesTest *createSuite() { return new MillerIndicesTest(); }
    static void destroySuite( MillerIndicesTest *suite ) { delete suite; }

    void testdirectAccess()
    {
        MillerIndices hkl(1, 1, 0);

        TS_ASSERT_EQUALS(hkl.h(), 1);
        TS_ASSERT_EQUALS(hkl.k(), 1);
        TS_ASSERT_EQUALS(hkl.l(), 0);
    }

    void testoperatorAccess()
    {
        MillerIndices hkl(1, 1, 0);

        TS_ASSERT_EQUALS(hkl[0], 1);
        TS_ASSERT_EQUALS(hkl[1], 1);
        TS_ASSERT_EQUALS(hkl[2], 0);

        TS_ASSERT_THROWS(hkl[-2], std::range_error);
        TS_ASSERT_THROWS(hkl[3], std::range_error);
    }

    void testvectorAccess()
    {
        MillerIndices hkl(1, 1, 0);

        std::vector<int> hklVector = hkl.asVector();

        TS_ASSERT_EQUALS(hklVector.size(), 3);
        TS_ASSERT_EQUALS(hklVector[0], 1);
        TS_ASSERT_EQUALS(hklVector[1], 1);
        TS_ASSERT_EQUALS(hklVector[2], 0);
    }

    void testcopy()
    {
        MillerIndices hkl(1, 1, 0);
        MillerIndices copy = hkl;

        std::vector<int> copyVector = copy.asVector();

        TS_ASSERT_EQUALS(copyVector.size(), 3);
        TS_ASSERT_EQUALS(copyVector[0], hkl[0]);
    }
};

#endif // MANTID_SINQ_MILLERINDICESTEST_H
