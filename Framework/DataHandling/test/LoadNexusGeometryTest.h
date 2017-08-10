#ifndef LOAD_NEXUS_GEOMETRY_TEST_H_
#define LOAD_NEXUS_GEOMETRY_TEST_H_

#include "MantidDataHandling/LoadNexusGeometry.h"

#include <cxxtest/TestSuite.h>

using namespace Mantid::DataHandling;

class LoadNexusGeometryTest : public CxxTest::TestSuite
{
public:
    void testExecLoadNexusGeometry()
    {
        LoadNexusGeometry ld;
        TS_ASSERT_THROWS_NOTHING(ld.initialize ());
        TS_ASSERT_THROWS_NOTHING(ld.execute ())
        TS_ASSERT(ld.isExecuted ())
    }
};

#endif // LOAD_NEXUS_GEOMETRY_TEST_H_
