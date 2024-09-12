// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/Workspace.h"
#include "MantidDataHandling/ScaleInstrumentComponent.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/Component.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidKernel/Exception.h"
#include <Eigen/Dense>
#include <cxxtest/TestSuite.h>
#include <vector>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;
using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;

class ScaleInstrumentComponentTest : public CxxTest::TestSuite {
public:
  static ScaleInstrumentComponentTest *createSuite() { return new ScaleInstrumentComponentTest(); }
  static void destroySuite(ScaleInstrumentComponentTest *suite) { delete suite; }

  ScaleInstrumentComponentTest() {
    // instrument.reset(new Instrument);
    // wsName = "tst";

    // instrument->add(createBankWithDetector());
    // instrument->markAsDetector(det1);

    // WS.reset(new Workspace2D());
    // WS->setInstrument(instrument);
    // AnalysisDataService::Instance().add(wsName, WS);
  }

  void setUp() override {
    printf("Setting up\n");
    WS.reset(new Workspace2D());
    wsName = "tst";
    instrument.reset(new Instrument);
    instrument->add(createBankWithDetector());
    instrument->markAsDetector(det1);
    WS->setInstrument(instrument);
    AnalysisDataService::Instance().add(wsName, WS);
  }

  void tearDown() override {
    printf("Tearing down\n");
    AnalysisDataService::Instance().remove(wsName);
  }

  void testSimple() {
    IComponent_const_sptr det = WS->getInstrument()->getDetector(det_id);
    Eigen::Vector3d detPos(det->getPos().X(), det->getPos().Y(), det->getPos().Z());
    Eigen::Vector3d scalings(2.0, 2.0, 2.0);

    ScaleInstrumentComponent algo;
    algo.initialize();
    algo.setPropertyValue("Workspace", wsName);
    algo.setPropertyValue("ComponentName", bank_name);
    algo.setPropertyValue("Scalings", "2.0, 2.0, 2.0");
    algo.setPropertyValue("ScalePixelSizes", "1");
    algo.execute();

    checkExpectedDetectorPosition(scalings, detPos);
  }

  void testNoScalePixelSizes() {
    IComponent_const_sptr det = WS->getInstrument()->getDetector(det_id);
    Eigen::Vector3d detPos(det->getPos().X(), det->getPos().Y(), det->getPos().Z());
    Eigen::Vector3d scalings(2.0, 2.0, 2.0);
    auto scalePixels = false;

    ScaleInstrumentComponent algo;
    algo.initialize();
    algo.setPropertyValue("Workspace", wsName);
    algo.setPropertyValue("ComponentName", bank_name);
    algo.setPropertyValue("Scalings", "2.0, 2.0, 2.0");
    algo.setPropertyValue("ScalePixelSizes", "0");
    algo.execute();

    checkExpectedDetectorPosition(scalings, detPos, scalePixels);
  }

  void testScaleDetector() {
    IComponent_const_sptr det = WS->getInstrument()->getDetector(det_id);
    Eigen::Vector3d detPos(det->getPos().X(), det->getPos().Y(), det->getPos().Z());
    Eigen::Vector3d scalings(2.0, 2.0, 2.0);

    ScaleInstrumentComponent algo;
    algo.initialize();
    algo.setPropertyValue("Workspace", wsName);
    algo.setPropertyValue("ComponentName", (bank_name + "/" + det_name));
    algo.setPropertyValue("Scalings", "2.0, 2.0, 2.0");
    algo.setPropertyValue("ScalePixelSizes", "1");
    TS_ASSERT_THROWS(algo.execute(), const std::runtime_error &);
  }

private:
  std::string wsName;
  Detector *det1;
  const int det_id = 1;
  const std::string det_name = "det1";
  const std::string bank_name = "bank";
  std::shared_ptr<Instrument> instrument;
  MatrixWorkspace_sptr WS;

  CompAssembly *createBankWithDetector() {
    CompAssembly *bank = new CompAssembly(bank_name);
    bank->setPos(1., 0, 1.);
    Quat q(0.9, 0, 0, 0.2);
    q.normalize();
    bank->setRot(q);

    det1 = new Detector(det_name, det_id, nullptr);
    det1->setPos(1.0, 1.0, 1.0);
    bank->add(det1);

    return bank;
  }

  void checkExpectedDetectorPosition(const Eigen::Vector3d &scalings, const Eigen::Vector3d &detPos,
                                     const bool scalePixels = true) {
    printf("Checking expected detector position\n");
    Eigen::Matrix3d scalingMatrix = scalings.asDiagonal();
    IComponent_const_sptr bank = WS->getInstrument()->getComponentByName(bank_name);
    Eigen::Vector3d bank_pos(bank->getPos().X(), bank->getPos().Y(), bank->getPos().Z());
    IComponent_const_sptr det = WS->getInstrument()->getDetector(det_id);
    Eigen::Vector3d expectedPos = scalingMatrix * detPos + (Eigen::Matrix3d::Identity() - scalingMatrix) * bank_pos;

    TS_ASSERT_EQUALS(det->getPos().X(), expectedPos(0));
    TS_ASSERT_EQUALS(det->getPos().Y(), expectedPos(1));
    TS_ASSERT_EQUALS(det->getPos().Z(), expectedPos(2));
    TS_ASSERT_EQUALS(bank->getPos().X(), 1.0);
    TS_ASSERT_EQUALS(bank->getPos().Y(), 0.0);
    TS_ASSERT_EQUALS(bank->getPos().Z(), 1.0);

    auto scale = det->getScaleFactor();
    if (scalePixels) {
      TS_ASSERT_EQUALS(scale.X(), scalings(0));
      TS_ASSERT_EQUALS(scale.Y(), scalings(1));
      TS_ASSERT_EQUALS(scale.Z(), scalings(2));
    } else {
      TS_ASSERT_EQUALS(scale.X(), 1.0);
      TS_ASSERT_EQUALS(scale.Y(), 1.0);
      TS_ASSERT_EQUALS(scale.Z(), 1.0);
    }
  }
};
