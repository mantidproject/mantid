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
#include "MantidTestHelpers/AlgorithmHelper.h"

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

  void setUp()
  {
    Mantid::Kernel::ConfigService::Instance().setString("default.facility", "TEST");
  }
    
  void test_Init()
  {
    MakeDiffractionMDEventWorkspace alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }
  


  /** Test various combinations of OutputDimensions parameter */
  void test_OutputDimensions_Parameter()
  {
    EventWorkspace_sptr in_ws = createDiffractionEventWorkspace(10);
    AnalysisDataService::Instance().addOrReplace("testInEW", in_ws);
    Algorithm_sptr alg;

    alg = AlgorithmHelper::runAlgorithm("MakeDiffractionMDEventWorkspace", 6,
        "InputWorkspace", "testInEW",
        "OutputWorkspace", "testOutMD",
        "OutputDimensions", "Q (lab frame)");
    TS_ASSERT( alg->isExecuted() );

    MDEventWorkspace3::sptr ws;
    TS_ASSERT_THROWS_NOTHING( ws = boost::dynamic_pointer_cast<MDEventWorkspace3>(AnalysisDataService::Instance().retrieve("testOutMD")) );
    TS_ASSERT(ws);
    if (!ws) return;
    TS_ASSERT_EQUALS( ws->getDimension(0)->getName(), "Qx");

    // But you can't add to an existing one of the wrong dimensions type
    alg = AlgorithmHelper::runAlgorithm("MakeDiffractionMDEventWorkspace", 6,
        "InputWorkspace", "testInEW",
        "OutputWorkspace", "testOutMD",
        "OutputDimensions", "HKL");
    TS_ASSERT( !alg->isExecuted() );

    // Let's try again - it will work.
    AnalysisDataService::Instance().remove("testOutMD");
    alg = AlgorithmHelper::runAlgorithm("MakeDiffractionMDEventWorkspace", 6,
        "InputWorkspace", "testInEW",
        "OutputWorkspace", "testOutMD",
        "OutputDimensions", "HKL");
    TS_ASSERT( alg->isExecuted() );

    TS_ASSERT_THROWS_NOTHING( ws = boost::dynamic_pointer_cast<MDEventWorkspace3>(AnalysisDataService::Instance().retrieve("testOutMD")) );
    TS_ASSERT(ws);
    if (!ws) return;
    TS_ASSERT_EQUALS( ws->getDimension(0)->getName(), "H");
  }




  void do_test_MINITOPAZ(EventType type, size_t numTimesToAdd = 1)
  {

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
    for (size_t i=1; i < numTimesToAdd; i++)
    {
      std::cout << "Iteration " << i << std::endl;
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

      TS_ASSERT_EQUALS( npoints*(i+1), ws->getNPoints()); // There are now twice as many points as before
    }



    AnalysisDataService::Instance().remove("test_md3");
  }

  void test_MINITOPAZ()
  {
    do_test_MINITOPAZ(TOF);
  }
//
//  void test_MINITOPAZ_weightedEvents()
//  {
//    do_test_MINITOPAZ(WEIGHTED);
//  }
//
//  void test_MINITOPAZ_weightedEvents_noTime()
//  {
//    do_test_MINITOPAZ(WEIGHTED);
//  }
//
//  void test_MINITOPAZ_addToExistingWorkspace()
//  {
//    do_test_MINITOPAZ(TOF, 2);
//  }
//
//  void test_MINITOPAZ_forProfiling()
//  {
//    do_test_MINITOPAZ(TOF, 100);
//  }



};


#endif /* MANTID_MDEVENTS_MAKEDIFFRACTIONMDEVENTWORKSPACETEST_H_ */

