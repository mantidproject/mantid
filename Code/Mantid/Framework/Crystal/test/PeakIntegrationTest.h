#ifndef MANTID_CRYSTAL_PeakIntegrationTEST_H_
#define MANTID_CRYSTAL_PeakIntegrationTEST_H_

#include "MantidDataHandling/LoadInstrument.h"
#include "MantidCrystal/PeakIntegration.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include "MantidTestHelpers/AlgorithmHelper.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <boost/random/linear_congruential.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int.hpp>
#include <boost/random/uniform_real.hpp>
#include <boost/random/variate_generator.hpp>
#include <math.h>
#include <cxxtest/TestSuite.h>
#include <iomanip>
#include <iostream>

using namespace Mantid;
using namespace Mantid::Crystal;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::DataHandling;
using namespace Mantid::Geometry;


class PeakIntegrationTest : public CxxTest::TestSuite
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
    int numBins = 16;
    double binDelta = 10.0;
    boost::mt19937 rng;
    boost::uniform_real<double> u2(0, 1.0); // Random from 0 to 1.0
    boost::variate_generator<boost::mt19937&, boost::uniform_real<double> > genUnit(rng, u2);
    int randomSeed = 1;
    rng.seed((unsigned int)(randomSeed));
    size_t nd = 1;
    // Make a random generator for each dimensions
    typedef boost::variate_generator<boost::mt19937&, boost::uniform_real<double> >   gen_t;
    gen_t * gens[1];
    for (size_t d=0; d<nd; ++d)
    {
      double min = -1.;
      double max = 1.;
      if (max <= min) throw std::invalid_argument("UniformParams: min must be < max for all dimensions.");
      boost::uniform_real<double> u(min,max); // Range
      gen_t * gen = new gen_t(rng, u);
      gens[d] = gen;
    }



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
      //Background
      for (int i=0; i<numBins; i++)
      {
        //Two events per bin
        retVal->getEventListAtPixelID(pix) += TofEvent((i+0.5)*binDelta, run_start+double(i));
        retVal->getEventListAtPixelID(pix) += TofEvent((i+0.5)*binDelta, run_start+double(i));
      }

      //Peak
      int r = static_cast<int>(numEvents/std::sqrt((pix/100-50.5)*(pix/100-50.5) + (pix%100-50.5)*(pix%100-50.5)));
      for (int i=0; i<r; i++)
      {
        retVal->getEventListAtPixelID(pix) += TofEvent(5844.+10.*(((*gens[0])()+(*gens[0])()+(*gens[0])())*2.-3.), run_start+double(i));
      }

    }

    retVal->doneLoadingData();
    /// Clean up the generators
    for (size_t d=0; d<nd; ++d)
      delete gens[d];


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
    PeakIntegration alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }
  


  void do_test_MINITOPAZ(EventType type)
  {

    int numEventsPer = 100;
    MatrixWorkspace_sptr inputW = createDiffractionEventWorkspace(numEventsPer);
    EventWorkspace_sptr in_ws = boost::dynamic_pointer_cast<EventWorkspace>( inputW );
    inputW->getAxis(0)->setUnit("TOF");
    if (type == WEIGHTED)
      in_ws *= 2.0;
    if (type == WEIGHTED_NOTIME)
    {
      for (size_t i =0; i<in_ws->getNumberHistograms(); i++)
      {
        EventList & el = in_ws->getEventList(i);
        el.compressEvents(0.0, &el);
      }
    }
    // Register the workspace in the data service

    // Create the peaks workspace
    PeaksWorkspace_sptr pkws(new PeaksWorkspace());
    pkws->setName("TOPAZ");

    // This loads (appends) the peaks
    Peak PeakObj(in_ws->getInstrument(),5050,2.,V3D(1,1,1));
    pkws->addPeak( PeakObj);
    AnalysisDataService::Instance().add("TOPAZ", pkws);

    PeakIntegration alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    alg.setProperty("InputWorkspace", inputW);
    alg.setProperty("OutputWorkspace", "tmp");
    alg.setProperty("InPeaksWorkspace", "TOPAZ");
    alg.setProperty("XMin", -2);
    alg.setProperty("XMax", 2);
    alg.setProperty("YMin", -2);
    alg.setProperty("YMax", 2);
    alg.setProperty("TOFBinMin", -5);
    alg.setProperty("TOFBinMax", 5);
    alg.setProperty("Params", "5760.,10.0,5920.");
    TS_ASSERT_THROWS_NOTHING( alg.execute(); )
    TS_ASSERT( alg.isExecuted() )

    PeaksWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING(
        ws = boost::dynamic_pointer_cast<PeaksWorkspace>(AnalysisDataService::Instance().retrieve("TOPAZ")) );
    TS_ASSERT(ws);
    if (!ws) return;
    Peak & peak = ws->getPeaks()[0];
    TS_ASSERT_DELTA( peak.getIntensity(), 1348.4223, 10.0);
    TS_ASSERT_DELTA( peak.getSigmaIntensity(), 44.6417, 1.0);
    AnalysisDataService::Instance().remove("TOPAZ");

  }

  void test_MINITOPAZ()
  {
    do_test_MINITOPAZ(TOF);
  }



};


#endif /* MANTID_CRYSTAL_PeakIntegrationTEST_H_ */
