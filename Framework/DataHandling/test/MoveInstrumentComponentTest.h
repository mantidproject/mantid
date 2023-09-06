// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/Workspace.h"
#include "MantidDataHandling/MoveInstrumentComponent.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/Component.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidKernel/Exception.h"
#include <vector>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;
using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;

class MoveInstrumentComponentTest : public CxxTest::TestSuite {
public:
  static MoveInstrumentComponentTest *createSuite() { return new MoveInstrumentComponentTest(); }
  static void destroySuite(MoveInstrumentComponentTest *suite) { delete suite; }

  MoveInstrumentComponentTest() {
    // initialise framework manager to allow logging
    // Mantid::API::FrameworkManager::Instance().initialize();

    instrument.reset(new Instrument);
    wsName = "tst";

    instrument->add(createBankWithDetector());
    instrument->markAsDetector(det1);

    WS.reset(new Workspace2D());
    WS->setInstrument(instrument);
    AnalysisDataService::Instance().add(wsName, WS);
  }
  void testRelative() {
    MoveInstrumentComponent mover;
    mover.initialize();
    mover.setPropertyValue("Workspace", wsName);
    mover.setPropertyValue("DetectorID", std::to_string(det_id));
    mover.setPropertyValue("X", "10");
    mover.setPropertyValue("Y", "20");
    mover.setPropertyValue("Z", "30");
    mover.execute();

    V3D expectedPos = det1->getPos() + V3D(10, 20, 30);
    checkExpectedDetectorPosition(expectedPos);
  }

  void testAbsolute() {
    MoveInstrumentComponent mover;
    mover.initialize();
    mover.setPropertyValue("Workspace", wsName);
    mover.setPropertyValue("DetectorID", std::to_string(det_id));
    mover.setPropertyValue("X", "10");
    mover.setPropertyValue("Y", "20");
    mover.setPropertyValue("Z", "30");
    mover.setPropertyValue("RelativePosition", "0");
    mover.execute();

    V3D expectedPos = V3D(10, 20, 30);
    checkExpectedDetectorPosition(expectedPos);
  }

  void testMoveByName() {
    MoveInstrumentComponent mover;
    mover.initialize();
    mover.setPropertyValue("Workspace", wsName);
    mover.setPropertyValue("ComponentName", det_name);
    mover.setPropertyValue("X", "10");
    mover.setPropertyValue("Y", "20");
    mover.setPropertyValue("Z", "30");
    mover.setPropertyValue("RelativePosition", "0");
    mover.execute();

    V3D expectedPos = V3D(10, 20, 30);
    checkExpectedDetectorPosition(expectedPos);
  }

  void testMoveByFullName() {
    MoveInstrumentComponent mover;
    mover.initialize();
    mover.setPropertyValue("Workspace", wsName);
    mover.setPropertyValue("ComponentName", (bank_name + "/" + det_name));
    mover.setPropertyValue("X", "10");
    mover.setPropertyValue("Y", "20");
    mover.setPropertyValue("Z", "30");
    mover.setPropertyValue("RelativePosition", "0");
    mover.execute();

    V3D expectedPos = V3D(10, 20, 30);
    checkExpectedDetectorPosition(expectedPos);
  }

  void testMoveDetectorInStructuredBankIgnoredByDefault() {
    setupInstrumentWithRectangularDetector();

    MoveInstrumentComponent mover;
    mover.initialize();
    mover.setPropertyValue("Workspace", wsName);
    mover.setPropertyValue("DetectorID", std::to_string(det_id));
    mover.setPropertyValue("X", "10");
    mover.setPropertyValue("Y", "20");
    mover.setPropertyValue("Z", "30");
    mover.execute();

    V3D expectedPos = det1->getPos();
    checkExpectedDetectorPosition(expectedPos);
  }

  void testMoveDetectorInStructuredBank() {
    setupInstrumentWithRectangularDetector();

    MoveInstrumentComponent mover;
    mover.initialize();
    mover.setPropertyValue("Workspace", wsName);
    mover.setPropertyValue("DetectorID", std::to_string(det_id));
    mover.setPropertyValue("X", "10");
    mover.setPropertyValue("Y", "20");
    mover.setPropertyValue("Z", "30");
    mover.setPropertyValue("RelativePosition", "0");
    mover.setPropertyValue("MoveFixedDetectors", "1");
    mover.execute();

    V3D expectedPos = V3D(10, 20, 30);
    checkExpectedDetectorPosition(expectedPos);
  }

private:
  std::string wsName;
  Detector *det1;
  const int det_id = 1;
  const std::string det_name = "det1";
  const std::string bank_name = "bank";
  std::shared_ptr<Instrument> instrument;
  MatrixWorkspace_sptr WS;

  void setupInstrumentWithRectangularDetector() {
    instrument.reset(new Instrument);

    RectangularDetector *rect_det = new RectangularDetector("rectangular_detector");
    rect_det->setPos(1., 0, 1.);
    Quat rect_q(0.9, 0, 0, 0.2);
    rect_q.normalize();
    rect_det->setRot(rect_q);
    rect_det->add(createBankWithDetector());

    instrument->add(rect_det);
    instrument->markAsDetector(det1);

    WS->setInstrument(instrument);
  }

  CompAssembly *createBankWithDetector() {
    CompAssembly *bank = new CompAssembly(bank_name);
    bank->setPos(1., 0, 1.);
    Quat q(0.9, 0, 0, 0.2);
    q.normalize();
    bank->setRot(q);

    det1 = new Detector(det_name, det_id, nullptr);
    det1->setPos(1.0, 0.0, 0.0);
    bank->add(det1);

    return bank;
  }

  void checkExpectedDetectorPosition(const V3D &expectedPos) {
    IComponent_const_sptr det = WS->getInstrument()->getDetector(det_id);
    TS_ASSERT_EQUALS(det->getPos(), expectedPos)
  }
};
