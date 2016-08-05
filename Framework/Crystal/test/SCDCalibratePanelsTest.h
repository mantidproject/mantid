/*
 * SCDCalibratePanelsTest.h
 *
 *  Created on: Mar 12, 2012
 *      Author: ruth
 */

#ifndef SCDCALIBRATEPANELSTEST_H_
#define SCDCALIBRATEPANELSTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidCrystal/SCDCalibratePanels.h"

using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace std;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;
using namespace Mantid::Crystal;

class SCDCalibratePanelsTest : public CxxTest::TestSuite {

public:
  void test_TOPAZ_5637() {
    // load a peaks file
    boost::shared_ptr<Algorithm> alg =
        AlgorithmFactory::Instance().create("LoadIsawPeaks", 1);
    alg->initialize();
    alg->setPropertyValue("Filename", "Peaks5637.integrate");
    alg->setPropertyValue("OutputWorkspace", "TOPAZ_5637");
    TS_ASSERT(alg->execute());

    // run the calibration
    alg = AlgorithmFactory::Instance().create("SCDCalibratePanels", 1);
    alg->initialize();
    // Peakws->setName("PeaksWsp");
    alg->setPropertyValue("PeakWorkspace", "TOPAZ_5637");
    alg->setProperty("a", 4.75);
    alg->setProperty("b", 4.75);
    alg->setProperty("c", 13.0);
    alg->setProperty("alpha", 90.0);
    alg->setProperty("beta", 90.0);
    alg->setProperty("gamma", 120.0);
    alg->setPropertyValue("DetCalFilename", "/tmp/topaz.detcal"); // deleteme
    TS_ASSERT(alg->execute());

    // verify the results
    ITableWorkspace_sptr results =
        AnalysisDataService::Instance().retrieveWS<ITableWorkspace>("params_bank47");
    TS_ASSERT_DELTA(-0.00433, results->cell<double>(0, 1), 1e-4);
    TS_ASSERT_DELTA(0.00122, results->cell<double>(1, 1), 1e-4);
    TS_ASSERT_DELTA(0.00137, results->cell<double>(2, 1), 1e-4);
    TS_ASSERT_DELTA(-0.14099, results->cell<double>(3, 1), 1e-4);
    TS_ASSERT_DELTA(-0.11341, results->cell<double>(4, 1), 1e-4);
    TS_ASSERT_DELTA(0.16836, results->cell<double>(5, 1), 1e-4);
    TS_ASSERT_DELTA(1.00048, results->cell<double>(6, 1), 1e-4);
    TS_ASSERT_DELTA(1.00021, results->cell<double>(7, 1), 1e-4);
    TS_ASSERT_DELTA(0.30116, results->cell<double>(8, 1), 1e-4);
    ITableWorkspace_sptr resultsL1 =
        AnalysisDataService::Instance().retrieveWS<ITableWorkspace>("params_L1");
    TS_ASSERT_DELTA(-0.00761, resultsL1->cell<double>(2, 1), .01);
  }
};

#endif /* SCDCALIBRATEPANELSTEST_H_ */
