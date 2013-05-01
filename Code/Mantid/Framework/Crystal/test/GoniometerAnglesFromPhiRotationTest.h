/*
 * GoniometerAnglesFromPhiRotation.h
 *
 *  Created on: Apr 29, 2013
 *      Author: ruth
 */

#ifndef GoniometerAnglesFromPhiRotationTest_H_
#define GoniometerAnglesFromPhiRotationTest_H_

#include "MantidCrystal/GoniometerAnglesFromPhiRotation.h"

#include <cxxtest/TestSuite.h>
#include "MantidKernel/V3D.h"
#include "MantidGeometry/Crystal/IndexingUtils.h"
#include "MantidKernel/Quat.h"
#include "MantidKernel/Matrix.h"
#include "MantidGeometry/Instrument/Goniometer.h"
#include "MantidCrystal/LoadIsawPeaks.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "GoniometerAnglesFromPhiRotationTest.h"
#include "MantidAPI/AlgorithmFactory.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/FrameworkManager.h"

using namespace Mantid;
using Mantid::Crystal::LoadIsawPeaks;
using Mantid::DataObjects::PeaksWorkspace_sptr;
using Mantid::API::AlgorithmFactory;
using Mantid::API::Algorithm;
using Mantid::API::FrameworkManager;
using Mantid::Crystal::GoniometerAnglesFromPhiRotation;

class GoniometerAnglesFromPhiRotationTest: public CxxTest::TestSuite
{
public:

  void test_stuff()
  {
     LoadIsawPeaks loadPeaks;
     loadPeaks.initialize();
     loadPeaks.setProperty("FileName","Peaks5637.integrate");
     loadPeaks.setPropertyValue("OutputWorkspace","abc");
     loadPeaks.execute();
     loadPeaks.setPropertyValue("OutputWorkspace","abc");
     PeaksWorkspace_sptr Peak5637= loadPeaks.getProperty("OutputWorkspace");

     LoadIsawPeaks loadPeaks2;
     loadPeaks2.initialize();
     loadPeaks2.setProperty("FileName","Peaks5643.integrate");
     loadPeaks2.setPropertyValue("OutputWorkspace","def");
     loadPeaks2.execute();
     loadPeaks2.setPropertyValue("OutputWorkspace","def");
     PeaksWorkspace_sptr Peak5643= loadPeaks.getProperty("OutputWorkspace");

     GoniometerAnglesFromPhiRotation Gonr;
     //boost::shared_ptr<Algorithm> GonRot =Mantid::API::AlgorithmFactory::Instance().create("GoniometerAnglesFromPhiRotation");
     Gonr.initialize();
     Gonr.setPropertyValue("PeaksWorkspace1","abc");
     Gonr.setPropertyValue("PeaksWorkspace2","def");
     Gonr.setProperty("MIND",3.0);
     Gonr.setProperty("MAXD",15.0);
     Gonr.setProperty("Run1Phi", -.02);
     Gonr.setProperty("Phi2",45.0);
     Gonr.execute();

     TS_ASSERT_DELTA((int)Gonr.getProperty("Nindexed"),430,2);
     TS_ASSERT_DELTA((double)Gonr.getProperty("AvErrIndex"),0.0384297,.01);
     TS_ASSERT_DELTA((double)Gonr.getProperty("AvErrAll"),0.039254,.01);
     TS_ASSERT_DELTA((double)Gonr.getProperty("Phi2"),45,.5);
     TS_ASSERT_DELTA((double)Gonr.getProperty("Chi2"),135.069,.5);
     TS_ASSERT_DELTA((double)Gonr.getProperty("Omega2"),-3.0035,.5);

 /*    std::cout<< (int)Gonr.getProperty("Nindexed")<<","
           <<(double)Gonr.getProperty("AvErrIndex")<<","
           <<(double)Gonr.getProperty("AvErrAll")<<","
           <<(double)Gonr.getProperty("Phi2")<<","
           <<(double)Gonr.getProperty("Chi2")<<","
           <<(double)Gonr.getProperty("Omega2")<<std::endl;

//430,0.0384297,0.039254,45,135.069,-3.00351
*/



  }
};

#endif /* GoniometerAnglesFromPhiRotationTest_H_ */
