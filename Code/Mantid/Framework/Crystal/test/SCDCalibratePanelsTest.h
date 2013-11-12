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


class SCDCalibratePanelsTest : public CxxTest::TestSuite
{

public:

  void test_TOPAZ_3007()
  {
    // load a peaks file
    boost::shared_ptr< Algorithm > alg= AlgorithmFactory::Instance().create("LoadIsawPeaks", 1);
    alg->initialize() ;
    alg->setPropertyValue("Filename", "TOPAZ_3007.peaks");
    alg->setPropertyValue("OutputWorkspace", "TOPAZ_3007");
    TS_ASSERT(alg->execute());

    // run the calibration
    alg= AlgorithmFactory::Instance().create("SCDCalibratePanels", 1);
    alg->initialize();
    //Peakws->setName("PeaksWsp");
    alg->setPropertyValue("PeakWorkspace", "TOPAZ_3007" );
    alg->setProperty("a",14.0);
    alg->setProperty("b",19.3);
    alg->setProperty("c",  8.6);
    alg->setProperty("alpha",90.0);
    alg->setProperty("beta",105.0);
    alg->setProperty("gamma",90.0);
    alg->setProperty("RotateCenters", false);
    alg->setProperty("PanelGroups","SpecifyGroups");
    alg->setProperty("Grouping","26");
    alg->setPropertyValue("ResultWorkspace","Result");
    alg->setPropertyValue("QErrorWorkspace","QErrorResult");
    TS_ASSERT(alg->execute());

    // verify the results
    ITableWorkspace_sptr results
        = AnalysisDataService::Instance().retrieveWS< ITableWorkspace>("Result");
    TS_ASSERT_DELTA(-0.000561209,results->cell<double>(3,1),.01);
    TS_ASSERT_DELTA(6.6194e-06,results->cell<double>(2,1),.01);
    TS_ASSERT_DELTA(-4.99864,results->cell<double>(9,1),.01);
    TS_ASSERT_DELTA(18.0095,results->cell<double>(8,1),.01);

   }

};

#endif /* SCDCALIBRATEPANELSTEST_H_ */
