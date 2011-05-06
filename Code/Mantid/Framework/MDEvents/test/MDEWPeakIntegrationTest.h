#ifndef MANTID_MDEVENTS_MDEWPEAKINTEGRATIONTEST_H_
#define MANTID_MDEVENTS_MDEWPEAKINTEGRATIONTEST_H_

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidGeometry/IInstrument.h"
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include "MantidMDEvents/MDEventFactory.h"
#include "MantidMDEvents/MDEWPeakIntegration.h"
#include "MantidTestHelpers/AlgorithmHelper.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include <cxxtest/TestSuite.h>
#include <iomanip>
#include <iostream>

using Mantid::API::AnalysisDataService;
using Mantid::Geometry::MDHistoDimension;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;
using namespace Mantid::MDEvents;


class MDEWPeakIntegrationTest : public CxxTest::TestSuite
{
public:
  std::string outWSName;
    
  void test_Init()
  {
    MDEWPeakIntegration alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }
  

  void doRun(double PeakRadius)
  {
    MDEWPeakIntegration alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("InputWorkspace", outWSName ) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("PeaksWorkspace", "MDEWPeakIntegrationTest_peaks" ) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("CoordinatesToUse", "HKL" ) );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("PeakRadius", PeakRadius ) );
    TS_ASSERT_THROWS_NOTHING( alg.execute() );
    TS_ASSERT( alg.isExecuted() );
  }


  void test_exec()
  {
    outWSName = "MDEWPeakIntegrationTest_MDEWS";

    // ---- Start with empty MDEW ----
    AlgorithmHelper::runAlgorithm("CreateMDEventWorkspace", 16,
        "Dimensions", "3",
        "Extents", "-10,10,-10,10,-10,10",
        "Names", "h,k,l",
        "Units", "-,-,-",
        "BinarySplit", "0",
        "SplitInto", "5",
        "MaxRecursionDepth", "2",
        "OutputWorkspace", outWSName.c_str());

    // --- Give it a fake peak ------
    AlgorithmHelper::runAlgorithm("FakeMDEventData", 4,
        "InputWorkspace", outWSName.c_str(), "PeakParams", "1000, 0.,0.,0., 1.0");

    AlgorithmHelper::runAlgorithm("FakeMDEventData", 4,
        "InputWorkspace", outWSName.c_str(), "PeakParams", "1000, 2.,3.,4., 0.5");

    AlgorithmHelper::runAlgorithm("FakeMDEventData", 4,
        "InputWorkspace", outWSName.c_str(), "PeakParams", "1000, 5.,5.,5., 2.0");

    MDEventWorkspace3::sptr mdews = boost::dynamic_pointer_cast<MDEventWorkspace3>(AnalysisDataService::Instance().retrieve(outWSName));
    TS_ASSERT_EQUALS( mdews->getNPoints(), 3000);
    TS_ASSERT_DELTA( mdews->getBox()->getSignal(), 3000.0, 1e-2);

    // Make a fake instrument - doesn't matter, we won't use it really
    IInstrument_sptr inst = ComponentCreationHelper::createTestInstrumentCylindrical(5);

    // --- Make a fake PeaksWorkspace ---
    PeaksWorkspace_sptr peakWS(new PeaksWorkspace());
    peakWS->addPeak( Peak(inst, 1, 1.0, V3D(0., 0., 0.) ) );
    peakWS->addPeak( Peak(inst, 1, 1.0, V3D(2., 3., 4.) ) );
    peakWS->addPeak( Peak(inst, 1, 1.0, V3D(5., 5., 5.) ) );

    TS_ASSERT_EQUALS( peakWS->getPeak(0).getIntensity(), 0.0);
    AnalysisDataService::Instance().add("MDEWPeakIntegrationTest_peaks",peakWS);

    // ------------- Integrate with 1.0 radius ------------------------
    doRun(1.0);

    TS_ASSERT_DELTA( peakWS->getPeak(0).getIntensity(), 1000.0, 1e-2);
    TS_ASSERT_DELTA( peakWS->getPeak(1).getIntensity(), 1000.0, 1e-2);
    // Peak is of radius 2.0, but we get half that radius = 1/8th the volume
    TS_ASSERT_DELTA( peakWS->getPeak(2).getIntensity(),  125.0, 10);

    // ------------- Let's do it again with 2.0 radius ------------------------
    doRun(2.0);

    // All peaks are fully contained
    TS_ASSERT_DELTA( peakWS->getPeak(0).getIntensity(), 1000.0, 1e-2);
    TS_ASSERT_DELTA( peakWS->getPeak(1).getIntensity(), 1000.0, 1e-2);
    TS_ASSERT_DELTA( peakWS->getPeak(2).getIntensity(), 1000.0, 1e-2);

    // ------------- Let's do it again with 0.5 radius ------------------------
    doRun(0.5);

    TS_ASSERT_DELTA( peakWS->getPeak(0).getIntensity(), 125.0, 10);
    TS_ASSERT_DELTA( peakWS->getPeak(1).getIntensity(), 1000.0, 1e-2);
    TS_ASSERT_DELTA( peakWS->getPeak(2).getIntensity(), 15.0, 10);

    AnalysisDataService::Instance().remove(outWSName);
    AnalysisDataService::Instance().remove("MDEWPeakIntegrationTest_peaks");
  }


};


#endif /* MANTID_MDEVENTS_MDEWPEAKINTEGRATIONTEST_H_ */

