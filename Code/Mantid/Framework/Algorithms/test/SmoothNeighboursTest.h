#ifndef SmoothNeighboursTEST_H_
#define SmoothNeighboursTEST_H_

#include "MantidAlgorithms/SmoothNeighbours.h"
#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include <iostream>
#include <iomanip>

#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidDataHandling/LoadInstrument.h"
#include "MantidTestHelpers/AlgorithmHelper.h"

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::DataHandling;
using namespace Mantid::Algorithms;

class SmoothNeighboursTest : public CxxTest::TestSuite
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
  void do_test_MINITOPAZ(EventType type)
  {

    int numEventsPer = 100;
    MatrixWorkspace_sptr inputW = createDiffractionEventWorkspace(numEventsPer);
    EventWorkspace_sptr in_ws = boost::dynamic_pointer_cast<EventWorkspace>( inputW );
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
    size_t nevents0 = in_ws->getNumberEvents();
    // Register the workspace in the data service

    SmoothNeighbours alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    alg.setProperty("InputWorkspace", inputW);
    alg.setProperty("OutputWorkspace", "testEW");
    alg.setProperty("AdjX", 1);
    alg.setProperty("AdjY", 1);
    TS_ASSERT_THROWS_NOTHING( alg.execute(); )
    TS_ASSERT( alg.isExecuted() )

    EventWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING(
        ws = boost::dynamic_pointer_cast<EventWorkspace>(AnalysisDataService::Instance().retrieve("testEW")) );
    TS_ASSERT(ws);
    if (!ws) return;
    size_t nevents = ws->getNumberEvents();
    TS_ASSERT_LESS_THAN( nevents0, nevents);

    AnalysisDataService::Instance().remove("testEW");
  }

  void test_SmoothNeighbours()
  {
    do_test_MINITOPAZ(TOF);
  }

};

#endif /*SmoothNeighboursTEST_H_*/


