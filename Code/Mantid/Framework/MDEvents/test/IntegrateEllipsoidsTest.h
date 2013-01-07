#ifndef MANTID_MDEVENTS_INTEGRATE_ELLIPSOIDS_TEST_H_
#define MANTID_MDEVENTS_INTEGRATE_ELLIPSOIDS_TEST_H_

#include <iostream>
#include <fstream>


#include <cxxtest/TestSuite.h>
#include <Poco/File.h>
#include "MantidAPI/AnalysisDataService.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidMDEvents/IntegrateEllipsoids.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"


#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/AlgorithmFactory.h"

#include "MantidDataHandling/LoadNexus.h"
#include "MantidDataObjects/WorkspaceSingleValue.h"
#include "MantidDataHandling/LoadInstrument.h"

using Mantid::API::AnalysisDataService;
using Mantid::MDEvents::IntegrateEllipsoids;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;


class IntegrateEllipsoidsTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static IntegrateEllipsoidsTest *createSuite() { return new IntegrateEllipsoidsTest(); }
  static void destroySuite( IntegrateEllipsoidsTest *suite ) { delete suite; }


  void test_Init()
  {
    IntegrateEllipsoids alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }

  void test_exec()
  {                                            // expected results with size determined
                                               // automatically from projected event sigmas
    double inti_auto[] = { 88, 99, 23, 33, 8, 8, 4 };
    double sigi_auto[] = { 13.784, 18.1384, 13.1529, 9.94987, 5.83095, 10.2956, 10.2956 };
                                               // expected results with fixed size 
                                               // ellipsoids
    double inti_fixed[] = { 87.541, 95.3934, 21.3607, 33.4262, 7.36066, 9.68852, 3.54098 };
    double sigi_fixed[] = { 13.9656, 18.4523, 13.4335, 10.1106, 5.94223, 10.5231, 10.5375 };

                                               // first, load peaks into a peaks workspace
  
    Mantid::API::FrameworkManager::Instance(); // needed for LoadIsawPeaks
                                               // to be found (i.e. registered)
                                    
    std::string peaks_file("TOPAZ_3007.peaks");
    std::string peaks_ws_name("TOPAZ_3007_peaks");

    boost::shared_ptr<Mantid::API::Algorithm> peaks_loader = 
          Mantid::API::AlgorithmFactory::Instance().create(std::string("LoadIsawPeaks"), -1);
    peaks_loader->initialize();
    peaks_loader->setPropertyValue( "Filename", peaks_file );
    peaks_loader->setPropertyValue( "OutputWorkspace", peaks_ws_name );
    peaks_loader->execute();

                                               // next, load events into an event workspace
    std::string event_file("TOPAZ_3007_bank_37_20_sec.nxs");
    std::string event_ws_name("TOPAZ_3007_events");

    boost::shared_ptr<Mantid::API::Algorithm> event_loader =
          Mantid::API::AlgorithmFactory::Instance().create(std::string("LoadNexus"), -1);
    event_loader->initialize();

    event_loader->setPropertyValue( "FileName", event_file );
    event_loader->setPropertyValue( "OutputWorkspace", event_ws_name );
    event_loader->execute();
                                              // configure and test the algorithm
                                              // using automatically determined
                                              // ellipsoid sizes
    IntegrateEllipsoids integrator;
    TS_ASSERT_THROWS_NOTHING( integrator.initialize() );
    TS_ASSERT_THROWS_NOTHING( integrator.setPropertyValue( "InputWorkspace", event_ws_name ) );
    TS_ASSERT_THROWS_NOTHING( integrator.setPropertyValue( "PeaksWorkspace", peaks_ws_name ) );
    TS_ASSERT_THROWS_NOTHING( integrator.setPropertyValue( "RegionRadius", ".25" ) );
    TS_ASSERT_THROWS_NOTHING( integrator.setPropertyValue( "SpecifySize", "0" ) );
    TS_ASSERT_THROWS_NOTHING( integrator.setPropertyValue( "PeakSize", ".2" ) );
    TS_ASSERT_THROWS_NOTHING( integrator.setPropertyValue( "BackgroundInnerSize", ".2" ) );
    TS_ASSERT_THROWS_NOTHING( integrator.setPropertyValue( "BackgroundOuterSize", ".25" ) );
    TS_ASSERT_THROWS_NOTHING( integrator.setPropertyValue( "OutputWorkspace", peaks_ws_name ) );
    TS_ASSERT_THROWS_NOTHING( integrator.execute() );

    PeaksWorkspace_sptr peaks_ws;
    TS_ASSERT_THROWS_NOTHING( peaks_ws = boost::dynamic_pointer_cast<PeaksWorkspace>(
        AnalysisDataService::Instance().retrieve(peaks_ws_name)); );
    TS_ASSERT(peaks_ws);

    std::vector<Peak> & peaks = peaks_ws->getPeaks();
    for (size_t i = 13; i <= 19; i++ )
    {
      TS_ASSERT_DELTA( peaks[i].getIntensity(), inti_auto[i-13], 0.1 );
      TS_ASSERT_DELTA( peaks[i].getSigmaIntensity(), sigi_auto[i-13], 0.1 );
    }
                                              // configure and test the algorithm
                                              // using fixed ellipsoid sizes
    TS_ASSERT_THROWS_NOTHING( integrator.initialize() );
    TS_ASSERT_THROWS_NOTHING( integrator.setPropertyValue( "InputWorkspace", event_ws_name ) );
    TS_ASSERT_THROWS_NOTHING( integrator.setPropertyValue( "PeaksWorkspace", peaks_ws_name
 ) );
    TS_ASSERT_THROWS_NOTHING( integrator.setPropertyValue( "RegionRadius", ".25" ) );
    TS_ASSERT_THROWS_NOTHING( integrator.setPropertyValue( "SpecifySize", "1" ) );
    TS_ASSERT_THROWS_NOTHING( integrator.setPropertyValue( "PeakSize", ".2" ) );
    TS_ASSERT_THROWS_NOTHING( integrator.setPropertyValue( "BackgroundInnerSize", ".2" ) );
    TS_ASSERT_THROWS_NOTHING( integrator.setPropertyValue( "BackgroundOuterSize", ".25" ) );
    TS_ASSERT_THROWS_NOTHING( integrator.setPropertyValue( "OutputWorkspace", peaks_ws_name ) );
    TS_ASSERT_THROWS_NOTHING( integrator.execute() );

    peaks = peaks_ws->getPeaks();
    for (size_t i = 13; i <= 19; i++ )
    {
      TS_ASSERT_DELTA( peaks[i].getIntensity(), inti_fixed[i-13], 0.1 );
      TS_ASSERT_DELTA( peaks[i].getSigmaIntensity(), sigi_fixed[i-13], 0.1 );
    }

    AnalysisDataService::Instance().remove( event_ws_name );
    AnalysisDataService::Instance().remove(peaks_ws_name);
  }

};


#endif /* MANTID_MDEVENTS_INTEGRATE_ELLIPSOIDS_TEST_H_ */
