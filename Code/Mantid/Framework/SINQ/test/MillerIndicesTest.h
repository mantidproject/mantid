#ifndef MANTID_SINQ_MILLERINDICESTEST_H
#define MANTID_SINQ_MILLERINDICESTEST_H

#include <cxxtest/TestSuite.h>
#include "MantidSINQ/PoldiUtilities/MillerIndices.h"
#include <stdexcept>
#include "MantidKernel/V3D.h"

using namespace Mantid::Poldi;
using namespace Mantid::Kernel;

class MillerIndicesTest : public CxxTest::TestSuite
{
public:
    // This pair of boilerplate methods prevent the suite being created statically
    // This means the constructor isn't called when running other tests
    static MillerIndicesTest *createSuite() { return new MillerIndicesTest(); }
    static void destroySuite( MillerIndicesTest *suite ) { delete suite; }

    void testdefaultConstructor()
    {
        MillerIndices hkl;

        TS_ASSERT_EQUALS(hkl.h(), 0);
        TS_ASSERT_EQUALS(hkl.k(), 0);
        TS_ASSERT_EQUALS(hkl.l(), 0);
    }

    void testvectorConstructor()
    {
        std::vector<int> hkl;
        hkl.push_back(2);
        hkl.push_back(5);
        hkl.push_back(4);

        MillerIndices hklMI(hkl);
        TS_ASSERT_EQUALS(hklMI.h(), 2);
        TS_ASSERT_EQUALS(hklMI.k(), 5);
        TS_ASSERT_EQUALS(hklMI.l(), 4);

        hkl.push_back(3);

        TS_ASSERT_THROWS(MillerIndices fails(hkl), std::runtime_error);
    }

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

    void testAsV3D()
    {
        MillerIndices hkl(1, 1, 0);
        V3D v3d = hkl.asV3D();

        TS_ASSERT_EQUALS(v3d, V3D(1, 1, 0));
    }

    void testV3DConstructor()
    {
        MillerIndices hkl(V3D(1.0, 2.0, 3.0));

        TS_ASSERT_EQUALS(hkl.h(), 1);
        TS_ASSERT_EQUALS(hkl.k(), 2);
        TS_ASSERT_EQUALS(hkl.l(), 3);
    }

    void testComparison()
    {
        MillerIndices one(1, 1, 0);
        MillerIndices equal(1, 1, 0);
        MillerIndices unequal(1, 2, 3);

        TS_ASSERT(one == equal);
        TS_ASSERT(one != unequal);
        TS_ASSERT(equal != unequal);
    }

    void testEquality()
    {
        MillerIndices hklOne(1, 1, 0);
        MillerIndices hklTwo(1, 1, 0);
        MillerIndices hklThree(1, 2, 0);

        TS_ASSERT_EQUALS(hklOne, hklTwo);
        TS_ASSERT_DIFFERS(hklOne, hklThree);
    }
};

#endif // MANTID_SINQ_MILLERINDICESTEST_H
