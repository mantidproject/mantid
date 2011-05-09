#ifndef UPDATEINSTRUMENTTESTFROMFILE_H_
#define UPDATEINSTRUMENTTESTFROMFILE_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/UpdateInstrumentFromFile.h"
#include "MantidDataHandling/LoadInstrumentFromRaw.h"
#include "MantidDataHandling/LoadInstrument.h"
#include "MantidAPI/InstrumentDataService.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidGeometry/Instrument/Instrument.h"
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

  void testHRPD()
  {
    InstrumentDataService::Instance().remove("HRPD_Definition.xml");

    //create a workspace with some sample data and put in data service
    wsName = "LoadInstrumentTestHRPD";
    Workspace_sptr ws = WorkspaceFactory::Instance().create("Workspace2D",1,1,1);
    Workspace2D_sptr ws2D = boost::dynamic_pointer_cast<Workspace2D>(ws);
    TS_ASSERT_THROWS_NOTHING(AnalysisDataService::Instance().add(wsName, ws2D));

    // load instrument def file
    LoadInstrument loaderHRP;
    TS_ASSERT_THROWS_NOTHING(loaderHRP.initialize());
    loaderHRP.setPropertyValue("Filename", "IDFs_for_UNIT_TESTING/HRPD_for_UNIT_TESTING.xml");
    inputFile = loaderHRP.getPropertyValue("Filename");
    loaderHRP.setPropertyValue("Workspace", wsName);
    TS_ASSERT_THROWS_NOTHING(loaderHRP.execute());
    TS_ASSERT( loaderHRP.isExecuted() );

    // now try to reload in detector positions from raw file
    UpdateInstrumentFromFile loadRawPos;
    loadRawPos.initialize();
    loadRawPos.setPropertyValue("Filename", "HRP38692.raw");
    loadRawPos.setPropertyValue("Workspace", wsName);
    TS_ASSERT_THROWS_NOTHING(loadRawPos.execute());
    TS_ASSERT( loadRawPos.isExecuted() );

    // Get back the saved workspace
    MatrixWorkspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(wsName)));

    boost::shared_ptr<IInstrument> i = output->getInstrument();

    boost::shared_ptr<IDetector> ptrDet = i->getDetector(3100);
    TS_ASSERT_EQUALS( ptrDet->getName(), "Det0");
    TS_ASSERT_DELTA( ptrDet->getPos().X(), 0.0866,0.01);
    TS_ASSERT_DELTA( ptrDet->getPos().Z(), -0.9962,0.01);


    AnalysisDataService::Instance().remove(wsName);
  }



private:
  UpdateInstrumentFromFile loader;
  std::string inputFile;
  std::string wsName;

};

#endif /*UPDATEINSTRUMENTTESTFROMFILE_H_*/
