// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef DIFFRACTIONEVENTREADDETCALTEST_H_
#define DIFFRACTIONEVENTREADDETCALTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/Workspace.h"
#include "MantidDataHandling/LoadEmptyInstrument.h"
#include "MantidDataHandling/LoadIsawDetCal.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/Component.h"
#include "MantidGeometry/Instrument/FitParameter.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/Exception.h"
#include <Poco/File.h>
#include <cstring>
#include <fstream>
#include <vector>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;
using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;

namespace {
void loadEmptyInstrument(const std::string &filename,
                         const std::string &wsName) {
  LoadEmptyInstrument loaderCAL;

  loaderCAL.initialize();
  loaderCAL.isInitialized();
  loaderCAL.setPropertyValue("Filename", filename);
  loaderCAL.setPropertyValue("OutputWorkspace", wsName);
  loaderCAL.execute();
  TS_ASSERT(loaderCAL.isExecuted());
}
} // namespace

class LoadIsawDetCalTest : public CxxTest::TestSuite {
public:
  void checkPosition(IComponent_const_sptr det, const double x, const double y,
                     const double z) {
    if (det != nullptr) {
      const auto detPos = det->getPos();
      const V3D testPos(x, y, z);
      TS_ASSERT_EQUALS(detPos, testPos);
    } else {
      throw std::runtime_error("In checkPosition IComponent is nullptr");
    }
  }

  void checkRotation(IComponent_const_sptr det, const double w, const double a,
                     const double b, const double c) {
    if (det != nullptr) {
      const auto detRot = det->getRotation();
      const Quat testRot(w, a, b, c);
      TS_ASSERT_EQUALS(detRot, testRot);
    } else {
      throw std::runtime_error("In checkRotation IComponent is nullptr");
    }
  }

  void testMINITOPAZ() {
    const std::string wsName("testMINITOPAZ");
    loadEmptyInstrument("unit_testing/MINITOPAZ_Definition.xml", wsName);

    // generate test file
    std::string inputFile = "test.DetCal";
    std::ofstream out(inputFile.c_str());
    out << "5      1    256    256 50.1000 49.9000  0.2000  55.33   50.0000   "
           "16.7548  -16.7548  0.00011 -0.00002  1.00000  0.00000  1.00000  "
           "0.00000\n";
    out.close();

    LoadIsawDetCal testerCAL;

    TS_ASSERT_THROWS_NOTHING(testerCAL.initialize());
    TS_ASSERT_THROWS_NOTHING(testerCAL.isInitialized());
    testerCAL.setPropertyValue("InputWorkspace", wsName);
    testerCAL.setPropertyValue("Filename", inputFile);
    inputFile = testerCAL.getPropertyValue("Filename"); // with absolute path
    TS_ASSERT_THROWS_NOTHING(testerCAL.execute());
    TS_ASSERT(testerCAL.isExecuted());

    MatrixWorkspace_sptr output =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(wsName);

    // Get some stuff from the input workspace
    Instrument_const_sptr ins = output->getInstrument();

    IComponent_const_sptr det = ins->getComponentByName("bank1");
    TS_ASSERT(det != nullptr);
    checkPosition(det, 0.500000, 0.167548, -0.167548);
    checkRotation(det, 0.707146, -8.47033e-22, -0.707068, -7.53079e-13);

    // remove file created by this algorithm
    Poco::File(inputFile).remove();
    // Remove workspace
    AnalysisDataService::Instance().remove(wsName);
  }

  void checkSNAP(const std::string &wsName) {
    MatrixWorkspace_const_sptr output =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(wsName);

    // Get some stuff from the input workspace
    Instrument_const_sptr ins = output->getInstrument();

    IComponent_const_sptr det1 = ins->getComponentByName("bank1");
    TS_ASSERT(det1 != nullptr);
    checkPosition(det1, 0.532001, 0.167548, -0.167546);
    checkRotation(det1, 0.707107, 0., -0.707107, 0.);

    IComponent_const_sptr det10 = ins->getComponentByName("bank10");
    TS_ASSERT(det10 != nullptr);
    checkPosition(det10, 0.167548, 0.167548, 0.);
    checkRotation(det10, 1., 0., 0., 0.);
  }

  void testSNAP() {
    const std::string wsName("testSNAP");
    loadEmptyInstrument("SNAP_Definition_2011-09-07.xml", wsName);

    // run the actual algorithm - filenames are together
    LoadIsawDetCal testerCAL;
    TS_ASSERT_THROWS_NOTHING(testerCAL.initialize());
    TS_ASSERT_THROWS_NOTHING(testerCAL.isInitialized());
    testerCAL.setPropertyValue("InputWorkspace", wsName);
    testerCAL.setPropertyValue("Filename",
                               "SNAP_34172_low.DetCal,SNAP_34172_high.DetCal");
    TS_ASSERT_THROWS_NOTHING(testerCAL.execute());
    TS_ASSERT(testerCAL.isExecuted());

    checkSNAP(wsName);

    // Remove workspace
    AnalysisDataService::Instance().remove(wsName);
  }

  void testSNAP2() {
    const std::string wsName("testSNAP2");
    loadEmptyInstrument("SNAP_Definition_2011-09-07.xml", wsName);

    // run the actual algorithm - filenames are separated
    LoadIsawDetCal testerCAL;
    TS_ASSERT_THROWS_NOTHING(testerCAL.initialize());
    TS_ASSERT_THROWS_NOTHING(testerCAL.isInitialized());
    testerCAL.setPropertyValue("InputWorkspace", wsName);
    testerCAL.setPropertyValue("Filename", "SNAP_34172_low.DetCal");
    testerCAL.setPropertyValue("Filename2", "SNAP_34172_high.DetCal");
    TS_ASSERT_THROWS_NOTHING(testerCAL.execute());
    TS_ASSERT(testerCAL.isExecuted());

    checkSNAP(wsName);

    // Remove workspace
    AnalysisDataService::Instance().remove(wsName);
  }
};

class LoadIsawDetCalTestPerformance : public CxxTest::TestSuite {
public:
  static LoadIsawDetCalTestPerformance *createSuite() {
    return new LoadIsawDetCalTestPerformance();
  }
  static void destroySuite(LoadIsawDetCalTestPerformance *suite) {
    delete suite;
  }

  void setUp() override {
    loadEmptyInstrument("SNAP_Definition_2011-09-07.xml", wsName);

    testerCAL.initialize();
    testerCAL.setPropertyValue("InputWorkspace", wsName);
    testerCAL.setPropertyValue("Filename", inputFile);
  }

  void tearDown() override { AnalysisDataService::Instance().remove(wsName); }

  void testLoadIsawDetCalTestPerformance() { testerCAL.execute(); }

private:
  LoadIsawDetCal testerCAL;
  const std::string inputFile = "SNAP_34172_low.DetCal, SNAP_34172_high.DetCal";
  const std::string wsName = "testSNAP";
};

#endif /*DIFFRACTIONEVENTREADDETCALTEST_H_*/
