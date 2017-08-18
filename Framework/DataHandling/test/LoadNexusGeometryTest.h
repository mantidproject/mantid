#ifndef LOAD_NEXUS_GEOMETRY_TEST_H_
#define LOAD_NEXUS_GEOMETRY_TEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAPI/InstrumentDataService.h"
#include "MantidBeamline/ComponentInfo.h"
#include "MantidBeamline/DetectorInfo.h"
#include "MantidDataHandling/LoadNexusGeometry.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/InstrumentVisitor.h"

#include "Eigen/Core"

using namespace Mantid;
using namespace DataHandling;

class LoadNexusGeometryTest : public CxxTest::TestSuite
{
public:
    void testInit()
    {
        LoadNexusGeometry loader;
        TS_ASSERT(!loader.isInitialized ());
        loader.initialize ();
        TS_ASSERT(loader.isInitialized ());

        TS_ASSERT_THROWS_NOTHING(loader.setProperty("InstrumentName", "testInstrument"));
    }

    void testExec()
    {
        LoadNexusGeometry loader;
        loader.initialize();
        loader.setProperty("InstrumentName", "testInstrument");

        TS_ASSERT(!loader.isExecuted());
        TS_ASSERT(loader.execute());
        TS_ASSERT(loader.isExecuted());
    }

};



#endif // LOAD_NEXUS_GEOMETRY_TEST_H_
