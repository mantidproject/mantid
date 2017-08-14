#ifndef LOAD_NEXUS_GEOMETRY_TEST_H_
#define LOAD_NEXUS_GEOMETRY_TEST_H_

#include "MantidAPI/InstrumentDataService.h"
#include "MantidDataHandling/LoadNexusGeometry.h"
#include "MantidGeometry/Instrument.h"

#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace DataHandling;

class LoadNexusGeometryTest : public CxxTest::TestSuite
{
public:
    void testInit()
    {
        TS_ASSERT(!loader.isInitialized ());
        loader.initialize ();
        TS_ASSERT(loader.isInitialized ());
    }

    void testExec()
    {
        if(!loader.isInitialized ())
            loader.initialize ();
        //Initialize instrument name
        loader.setProperty("InstrumentName", testInstrumentName);

        // Test Execution
        TS_ASSERT(!loader.isExecuted ());

        TS_ASSERT_THROWS_NOTHING(loader.execute ());
        TS_ASSERT(loader.isExecuted ());
        //Test if instrument exists with correct name
        boost::shared_ptr<Geometry::Instrument> testInstrument;
        testInstrument = API::InstrumentDataService::Instance ().retrieve (testInstrumentName); 
        TS_ASSERT(testInstrument->getName () == testInstrumentName);
        //Test if instrument has source
        TS_ASSERT(testInstrument->isEmptyInstrument ());
        TS_ASSERT(testInstrument->getSource ()->getName () == "Source");

    }

private:
    LoadNexusGeometry loader;
    std::string testInstrumentName = "LoadNexusGeometryTestInstrument";
};

#endif // LOAD_NEXUS_GEOMETRY_TEST_H_
