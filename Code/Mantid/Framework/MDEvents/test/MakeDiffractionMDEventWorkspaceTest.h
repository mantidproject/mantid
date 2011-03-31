#ifndef MANTID_MDEVENTS_MAKEDIFFRACTIONMDEVENTWORKSPACETEST_H_
#define MANTID_MDEVENTS_MAKEDIFFRACTIONMDEVENTWORKSPACETEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include <iostream>
#include <iomanip>

#include "MantidMDEvents/MakeDiffractionMDEventWorkspace.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataHandling/LoadInstrument.h"

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::DataHandling;

using namespace Mantid::MDEvents;

class MakeDiffractionMDEventWorkspaceTest : public CxxTest::TestSuite
{
public:

  /** Create an EventWorkspace containing fake data
   * of single-crystal diffraction.
   *
   * @return EventWorkspace_sptr
   */
  EventWorkspace_sptr createDiffractionEventWorkspace(int numEvents)
  {
    int numPixels = 10000;
    int numBins = 1600;
    double binDelta = 10.0;

    EventWorkspace_sptr retVal(new EventWorkspace);
    retVal->initialize(numPixels,1,1);

    // --------- Load the instrument -----------
    LoadInstrument * loadInst = new LoadInstrument();
    loadInst->initialize();
    loadInst->setPropertyValue("Filename", "IDFs_for_UNIT_TESTING/MINITOPAZ_Definition.xml");
    loadInst->setProperty<MatrixWorkspace_sptr> ("Workspace", retVal);
    loadInst->execute();
    delete loadInst;
    // Populate the instrument parameters in this workspace - this works around a bug
    retVal->populateInstrumentParameters();

    DateAndTime run_start("2010-01-01");

    for (int pix = 0; pix < numPixels; pix++)
    {
      for (int i=0; i<numEvents; i++)
      {
        retVal->getEventListAtPixelID(pix) += TofEvent((i+0.5)*binDelta, run_start+double(i));
      }

    }
    retVal->doneLoadingData();

    //Create the x-axis for histogramming.
    MantidVecPtr x1;
    MantidVec& xRef = x1.access();
    xRef.resize(numBins);
    for (int i = 0; i < numBins; ++i)
    {
      xRef[i] = i*binDelta;
    }

    //Set all the histograms at once.
    retVal->setAllX(x1);

    // Some sanity checks
    TS_ASSERT_EQUALS( retVal->getInstrument()->getName(), "MINITOPAZ");
    std::map<int, Geometry::IDetector_sptr> dets;
    retVal->getInstrument()->getDetectors(dets);
    TS_ASSERT_EQUALS( dets.size(), 100*100);

    return retVal;
  }

    
  void test_Init()
  {
    MakeDiffractionMDEventWorkspace alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }
  
  void do_test_MINITOPAZ(EventType type, bool addTwice=false)
  {
    Mantid::Kernel::ConfigService::Instance().setString("default.facility", "TEST");

    int numEventsPer = 100;
    EventWorkspace_sptr in_ws = createDiffractionEventWorkspace(numEventsPer);
    if (type == WEIGHTED)
      in_ws *= 2.0;
    if (type == WEIGHTED_NOTIME)
    {
      for (int i =0; i<in_ws->getNumberHistograms(); i++)
      {
        EventList & el = in_ws->getEventList(i);
        el.compressEvents(0.0, &el);
      }
    }

    MakeDiffractionMDEventWorkspace alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    alg.setProperty("InputWorkspace", in_ws);
    alg.setPropertyValue("OutputWorkspace", "test_md3");
    TS_ASSERT_THROWS_NOTHING( alg.execute(); )
    TS_ASSERT( alg.isExecuted() )

    MDEventWorkspace3::sptr ws;
    TS_ASSERT_THROWS_NOTHING(
        ws = boost::dynamic_pointer_cast<MDEventWorkspace3>(AnalysisDataService::Instance().retrieve("test_md3")) );
    TS_ASSERT(ws);
    if (!ws) return;
    size_t npoints = ws->getNPoints();
    TS_ASSERT_LESS_THAN( 100000, npoints); // Some points are left

    // Add to an existing MDEW
    if (addTwice)
    {
      TS_ASSERT_THROWS_NOTHING( alg.initialize() )
      TS_ASSERT( alg.isInitialized() )
      alg.setProperty("InputWorkspace", in_ws);
      alg.setPropertyValue("OutputWorkspace", "test_md3");
      TS_ASSERT_THROWS_NOTHING( alg.execute(); )
      TS_ASSERT( alg.isExecuted() )

      TS_ASSERT_THROWS_NOTHING(
          ws = boost::dynamic_pointer_cast<MDEventWorkspace3>(AnalysisDataService::Instance().retrieve("test_md3")) );
      TS_ASSERT(ws);
      if (!ws) return;

      TS_ASSERT_EQUALS( npoints*2, ws->getNPoints()); // There are now twice as many points as before
    }



    AnalysisDataService::Instance().remove("test_md3");
  }

  void test_MINITOPAZ()
  {
    do_test_MINITOPAZ(TOF);
  }

  void test_MINITOPAZ_weightedEvents()
  {
    do_test_MINITOPAZ(WEIGHTED);
  }

  void test_MINITOPAZ_weightedEvents_noTime()
  {
    do_test_MINITOPAZ(WEIGHTED);
  }

  void test_MINITOPAZ_addToExistingWorkspace()
  {
    do_test_MINITOPAZ(TOF, true);
  }



};


#endif /* MANTID_MDEVENTS_MAKEDIFFRACTIONMDEVENTWORKSPACETEST_H_ */

