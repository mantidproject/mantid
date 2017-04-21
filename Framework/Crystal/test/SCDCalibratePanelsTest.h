/*
 * SCDCalibratePanelsTest.h
 *
 *  Created on: Mar 12, 2012
 *      Author: ruth
 */

#ifndef SCDCALIBRATEPANELSTEST_H_
#define SCDCALIBRATEPANELSTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAPI/AnalysisDataService.h"
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
        AnalysisDataService::Instance().retrieveWS<ITableWorkspace>(
            "params_bank47");
    // TODO: Some of the fit parameters that are below are extermly sensitive to
    // rounding errors in the algorithm LoadIsawPeaks and floating point math in
    // the instrument code. Ideally the assertions should be on something else.
    TS_ASSERT_DELTA(-0.0050, results->cell<double>(0, 1), 1e-3);
    TS_ASSERT_DELTA(0.0013, results->cell<double>(1, 1), 3e-4);
    TS_ASSERT_DELTA(0.0012, results->cell<double>(2, 1), 2e-4);
    TS_ASSERT_DELTA(0.0, results->cell<double>(3, 1), 1.0);
    TS_ASSERT_DELTA(0.0, results->cell<double>(4, 1), 1.0);
    TS_ASSERT_DELTA(0.1133, results->cell<double>(5, 1), 0.36);
    TS_ASSERT_DELTA(1.0024, results->cell<double>(6, 1), 3e-3);
    TS_ASSERT_DELTA(0.9986, results->cell<double>(7, 1), 1e-2);
    TS_ASSERT_DELTA(0.2710, results->cell<double>(8, 1), 0.2);
    ITableWorkspace_sptr resultsL1 =
        AnalysisDataService::Instance().retrieveWS<ITableWorkspace>(
            "params_L1");
    TS_ASSERT_DELTA(-0.00761, resultsL1->cell<double>(2, 1), .01);
  }
};

#endif /* SCDCALIBRATEPANELSTEST_H_ */
