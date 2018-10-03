/*
 * GoniometerAnglesFromPhiRotation.h
 *
 *  Created on: Apr 29, 2013
 *      Author: ruth
 */

#ifndef GoniometerAnglesFromPhiRotationTest_H_
#define GoniometerAnglesFromPhiRotationTest_H_

#include "MantidCrystal/GoniometerAnglesFromPhiRotation.h"

#include "GoniometerAnglesFromPhiRotationTest.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/AlgorithmFactory.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidCrystal/LoadIsawPeaks.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidGeometry/Crystal/IndexingUtils.h"
#include "MantidGeometry/Instrument/Goniometer.h"
#include "MantidKernel/Matrix.h"
#include "MantidKernel/Quat.h"
#include "MantidKernel/V3D.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid;
using Mantid::API::Algorithm;
using Mantid::API::AlgorithmFactory;
using Mantid::API::FrameworkManager;
using Mantid::Crystal::GoniometerAnglesFromPhiRotation;
using Mantid::Crystal::LoadIsawPeaks;
using Mantid::DataObjects::PeaksWorkspace_sptr;

class GoniometerAnglesFromPhiRotationTest : public CxxTest::TestSuite {
public:
  void test_stuff() {
    LoadIsawPeaks loadPeaks;
    loadPeaks.initialize();
    loadPeaks.setProperty("FileName", "Peaks5637.integrate");
    loadPeaks.setPropertyValue("OutputWorkspace", "abc");
    loadPeaks.execute();
    loadPeaks.setPropertyValue("OutputWorkspace", "abc");
    // Needed for ticket #6469
    try {
      API::Workspace_sptr ows = loadPeaks.getProperty("OutputWorkspace");
      PeaksWorkspace_sptr Peak5637 =
          boost::dynamic_pointer_cast<DataObjects::PeaksWorkspace>(ows);
    } catch (...) {
      PeaksWorkspace_sptr Peak5637 = loadPeaks.getProperty("OutputWorkspace");
    }

    LoadIsawPeaks loadPeaks2;
    loadPeaks2.initialize();
    loadPeaks2.setProperty("FileName", "Peaks5643.integrate");
    loadPeaks2.setPropertyValue("OutputWorkspace", "def");
    loadPeaks2.execute();
    loadPeaks2.setPropertyValue("OutputWorkspace", "def");
    // Needed for ticket #6469
    try {
      API::Workspace_sptr ows = loadPeaks2.getProperty("OutputWorkspace");
      PeaksWorkspace_sptr Peak5643 =
          boost::dynamic_pointer_cast<DataObjects::PeaksWorkspace>(ows);
    } catch (...) {
      PeaksWorkspace_sptr Peak5643 = loadPeaks.getProperty("OutputWorkspace");
    }

    GoniometerAnglesFromPhiRotation Gonr;

    Gonr.initialize();
    Gonr.setPropertyValue("PeaksWorkspace1", "abc");
    Gonr.setPropertyValue("PeaksWorkspace2", "def");
    Gonr.setProperty("MIND", 3.0);
    Gonr.setProperty("MAXD", 15.0);
    Gonr.setProperty("Run1Phi", -.02);
    Gonr.setProperty("Phi2", 45.0);
    Gonr.execute();

    TS_ASSERT_DELTA((int)Gonr.getProperty("Nindexed"), 430, 2);
    TS_ASSERT_DELTA((double)Gonr.getProperty("AvErrIndex"), 0.0384297, .01);
    TS_ASSERT_DELTA((double)Gonr.getProperty("AvErrAll"), 0.039254, .01);
    TS_ASSERT_DELTA((double)Gonr.getProperty("Phi2"), 45, .5);
    TS_ASSERT_DELTA((double)Gonr.getProperty("Chi2"), 135.069, .5);
    TS_ASSERT_DELTA((double)Gonr.getProperty("Omega2"), -3.0035, .5);
  }
};

#endif /* GoniometerAnglesFromPhiRotationTest_H_ */
