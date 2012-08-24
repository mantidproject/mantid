#ifndef UPDATEINSTRUMENTTESTFROMFILE_H_
#define UPDATEINSTRUMENTTESTFROMFILE_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/UpdateInstrumentFromFile.h"
#include "MantidDataHandling/LoadInstrumentFromNexus.h"
#include "MantidDataHandling/LoadInstrument.h"
#include "MantidAPI/InstrumentDataService.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidGeometry/Instrument.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/Algorithm.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;
using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;

class UpdateInstrumentFromFileTest : public CxxTest::TestSuite
{
public:
    static UpdateInstrumentFromFileTest *createSuite() { return new UpdateInstrumentFromFileTest(); }
  static void destroySuite(UpdateInstrumentFromFileTest *suite) { delete suite; }

  UpdateInstrumentFromFileTest()
  {
  }

  void testHRPD_With_Moving_Monitors()
  {
    runTest(true);
  }

  void testHRPD_Without_Moving_Monitors()
  {
    runTest(false);
  }

private:
  
  void runTest(const bool moveMonitors)
  {
    const std::string xmlFile("HRPD_for_UNIT_TESTING.xml");
    InstrumentDataServiceImpl & ids = InstrumentDataService::Instance();
    if( ids.doesExist(xmlFile) ) ids.remove(xmlFile);

    //create a workspace with some sample data and put in data service
    wsName = "LoadInstrumentTestHRPD";
    Workspace_sptr ws = WorkspaceFactory::Instance().create("Workspace2D",1,1,1);
    Workspace2D_sptr ws2D = boost::dynamic_pointer_cast<Workspace2D>(ws);
    TS_ASSERT_THROWS_NOTHING(AnalysisDataService::Instance().add(wsName, ws2D));

    // load instrument def file
    LoadInstrument loaderHRP;
    TS_ASSERT_THROWS_NOTHING(loaderHRP.initialize());
    loaderHRP.setPropertyValue("Filename", "IDFs_for_UNIT_TESTING/" + xmlFile);
    inputFile = loaderHRP.getPropertyValue("Filename");
    loaderHRP.setPropertyValue("Workspace", wsName);
    TS_ASSERT_THROWS_NOTHING(loaderHRP.execute());
    TS_ASSERT( loaderHRP.isExecuted() );

    // Get back the saved workspace
     MatrixWorkspace_sptr output;
     TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(wsName));

    Instrument_const_sptr i = output->getInstrument();

    boost::shared_ptr<const IDetector> ptrDet = i->getDetector(3100);
    TS_ASSERT_EQUALS( ptrDet->getName(), "Det0");
    TS_ASSERT_DELTA( ptrDet->getPos().X(), 0.1318,0.01);
    TS_ASSERT_DELTA( ptrDet->getPos().Z(), -1.8853,0.01);

    boost::shared_ptr<const IDetector> mon1 = i->getDetector(1001);
    TS_ASSERT(mon1->isMonitor());
    double r(-1.0), theta(-1.0), phi(-1.0);
    mon1->getPos().getSpherical(r, theta, phi);
    if( moveMonitors )
    {
      TS_ASSERT_DELTA(r, 0.6, 1e-08);
      TS_ASSERT_DELTA(theta, 180.0, 1e-08);
      TS_ASSERT_DELTA(phi, 0.0, 1e-08);
    }
    else
    {
      TS_ASSERT_DELTA(r, 0.6, 1e-08);
      TS_ASSERT_DELTA(theta, 180.0, 1e-08);
      TS_ASSERT_DELTA(phi, 0.0, 1e-08);
    }

    InstrumentDataService::Instance().remove(xmlFile);
    AnalysisDataService::Instance().remove(wsName);

  }



private:
  UpdateInstrumentFromFile loader;
  std::string inputFile;
  std::string wsName;

};

#endif /*UPDATEINSTRUMENTTESTFROMFILE_H_*/
