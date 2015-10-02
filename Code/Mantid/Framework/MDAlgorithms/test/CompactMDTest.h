#ifndef MANTID_MDALGORITHMS_COMPACTMD_H_
#define MANTID_MDALGORITHMS_COMPACTMD_H_
#include <cxxtest/TestSuite.h>
#include "MantidMDAlgorithms/CompactMD.h"
#include "MantidDataObjects/MDHistoWorkspace.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"

using Mantid::MDAlgorithms::CompactMD;
using namespace Mantid::API;

class CompactMDTest : public CxxTest::TestSuite
{
public:
    // This pair of boilerplate methods prevent the suite being created statically
    // This means the constructor isn't called when running other tests
    static CompactMDTest *createSuite(){return new CompactMDTest();}
    static void destroySuite(CompactMDTest *suite) {delete suite;}

    void test_Init()
    {
        CompactMD alg;
        TSM_ASSERT_THROWS_NOTHING(alg.initialize() );
        TS_ASSERT( alg);
    }
};


#endif // !MANTID_MDALGORITHMS_COMPACTMD_H_

