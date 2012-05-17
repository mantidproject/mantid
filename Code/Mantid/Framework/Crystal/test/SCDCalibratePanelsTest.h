/*
 * SCDCalibratePanelsTest.h
 *
 *  Created on: Mar 12, 2012
 *      Author: ruth
 */

#ifndef SCDCALIBRATEPANELSTEST_H_
#define SCDCALIBRATEPANELSTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/AlgorithmFactory.h"
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidGeometry/IComponent.h"
#include "MantidKernel/Quat.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/V3D.h"
#include "MantidGeometry/Instrument/ParameterMap.h"
#include "MantidCrystal/SCDCalibratePanels.h"
#include "MantidDataObjects/Workspace2D.h"
//----------------------------------- For subtesting part of Calibrate Saving code------------------------
#include <iostream>
#include <fstream>
#include "MantidKernel/Property.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/AnalysisDataService.h"

using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace std;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;
using namespace Mantid::Crystal;
//------------------------------------------------


class SCDCalibratePanelsTest : public CxxTest::TestSuite
{

public:

  SCDCalibratePanelsTest(){}

  void test_data()
   {
     FrameworkManager::Instance();
     boost::shared_ptr< Algorithm > alg= AlgorithmFactory::Instance().create("LoadIsawPeaks", 1);

     alg->initialize() ;
     alg->setPropertyValue("Filename", "TOPAZ_3007.peaks");
     alg->setPropertyValue("OutputWorkspace", "TOPAZ_3007");

     alg->execute();

     PeaksWorkspace_sptr Peakws =boost::dynamic_pointer_cast<PeaksWorkspace>(
         AnalysisDataService::Instance().retrieve("TOPAZ_3007") );
     AnalysisDataService::Instance().remove("TOPAZ_3007");

     alg= AlgorithmFactory::Instance().create("SCDCalibratePanels", 1);

     alg->initialize();

     alg->setProperty("PeakWorkspace", Peakws );
     alg->setProperty("a",14.0);
     alg->setProperty("b",19.3);
     alg->setProperty("c",  8.6);
     alg->setProperty("alpha",90.0);
     alg->setProperty("beta",105.0);
     alg->setProperty("gamma",90.0);
    // alg->setProperty("use_L0", false);
    // alg->setProperty("use_timeOffset", false );
     alg->setProperty("XmlFilename","abc.xml");
     alg->setProperty("DetCalFilename","abc.DetCal");
     alg->setProperty("PanelGroups","SpecifyGroups");
     alg->setProperty("Grouping","26");

     alg->setPropertyValue("ResultWorkspace","Result");
     alg->setPropertyValue("QErrorWorkspace","QErrorResult");
     alg->execute();
     alg->setPropertyValue("ResultWorkspace","Result");

     boost::shared_ptr<TableWorkspace> Results  =  alg->getProperty("ResultWorkspace");
        // (AnalysisDataService::Instance().retrieveWS<TableWorkspace>(string("Result")));

     TS_ASSERT_DELTA(1.00001,Results->cell<double>(3,1),.01);
     TS_ASSERT_DELTA(0.998187,Results->cell<double>(2,1),.01);
     TS_ASSERT_DELTA(-0.416479,Results->cell<double>(9,1),.01);
     TS_ASSERT_DELTA(-0.298888,Results->cell<double>(8,1),.01);
     TS_ASSERT_DELTA(0.212273,Results->cell<double>(17,1),.01);

     /* for( int i=0; i<(int)Results->rowCount(); i++)
     {

       std::cout<<"row "<<i<<"="<<Results->cell<double>(i,1)<<std::endl;
     }
row 0=18.0095  row 1=-4.99949  row 2=0.998187 row 3=1.00001  row 4=1.07244e-07
row 5=-0.000569398 row 6=-0.000179413  row 7=-0.103256  row 8=-0.298888
row 9=-0.416479 row 10=0.129725 row 11=2.56685 row 12=0.287941 row 13=0.288195
row 14=0.0945599 row 15=0.0352754 row 16=0.0696436 row 17=0.212273 row 18=0.323982
row 19=1.12121
*/
   }

};

#endif /* SCDCALIBRATEPANELSTEST_H_ */
