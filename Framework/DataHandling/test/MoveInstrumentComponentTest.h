#ifndef MOVEINSTRUMENTCOMPONENTTEST_H_
#define MOVEINSTRUMENTCOMPONENTTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/WorkspaceFactory.h"
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
  static MoveInstrumentComponentTest *createSuite() {
    return new MoveInstrumentComponentTest();
  }
  static void destroySuite(MoveInstrumentComponentTest *suite) { delete suite; }

  MoveInstrumentComponentTest() {
    // initialise framework manager to allow logging
    // Mantid::API::FrameworkManager::Instance().initialize();

    instrument.reset(new Instrument);
    wsName = "tst";

    CompAssembly *bank = new CompAssembly("bank");
    bank->setPos(1., 0, 1.);
    Quat q(0.9, 0, 0, 0.2);
    q.normalize();
    bank->setRot(q);
    instrument->add(bank);

    det1 = new Detector("det1", 1, nullptr);
    det1->setPos(1.0, 0.0, 0.0);
    bank->add(det1);
    instrument->markAsDetector(det1);

    WS.reset(new Workspace2D());
    WS->setInstrument(instrument);
    AnalysisDataService::Instance().add(wsName, WS);
  }
  void testRelative() {
    MoveInstrumentComponent mover;
    mover.initialize();
    mover.setPropertyValue("Workspace", wsName);
    mover.setPropertyValue("DetectorID", "1");
    mover.setPropertyValue("X", "10");
    mover.setPropertyValue("Y", "20");
    mover.setPropertyValue("Z", "30");
    mover.execute();

    Instrument_const_sptr inst = WS->getInstrument();
    // get pointer to the first detector in the bank
    boost::shared_ptr<const IComponent> comp =
        (*boost::dynamic_pointer_cast<const ICompAssembly>(
            (*boost::dynamic_pointer_cast<const ICompAssembly>(inst))[0]))[0];

    V3D pos = comp->getPos();
    TS_ASSERT_EQUALS(pos, det1->getPos() + V3D(10, 20, 30))
  }

  void testAbsolute() {
    MoveInstrumentComponent mover;
    mover.initialize();
    mover.setPropertyValue("Workspace", wsName);
    mover.setPropertyValue("DetectorID", "1");
    mover.setPropertyValue("X", "10");
    mover.setPropertyValue("Y", "20");
    mover.setPropertyValue("Z", "30");
    mover.setPropertyValue("RelativePosition", "0");
    mover.execute();

    Instrument_const_sptr inst = WS->getInstrument();
    // get pointer to the first detector in the bank
    boost::shared_ptr<const IComponent> comp =
        (*boost::dynamic_pointer_cast<const ICompAssembly>(
            (*boost::dynamic_pointer_cast<const ICompAssembly>(inst))[0]))[0];

    V3D pos = comp->getPos();
    TS_ASSERT_EQUALS(pos, V3D(10, 20, 30))
  }

  void testMoveByName() {
    MoveInstrumentComponent mover;
    mover.initialize();
    mover.setPropertyValue("Workspace", wsName);
    mover.setPropertyValue("ComponentName", "det1");
    mover.setPropertyValue("X", "10");
    mover.setPropertyValue("Y", "20");
    mover.setPropertyValue("Z", "30");
    mover.setPropertyValue("RelativePosition", "0");
    mover.execute();

    Instrument_const_sptr inst = WS->getInstrument();
    // get pointer to the first detector in the bank
    boost::shared_ptr<const IComponent> comp =
        (*boost::dynamic_pointer_cast<const ICompAssembly>(
            (*boost::dynamic_pointer_cast<const ICompAssembly>(inst))[0]))[0];

    V3D pos = comp->getPos();
    TS_ASSERT_EQUALS(pos, V3D(10, 20, 30))
  }

  void testMoveByFullName() {
    MoveInstrumentComponent mover;
    mover.initialize();
    mover.setPropertyValue("Workspace", wsName);
    mover.setPropertyValue("ComponentName", "bank/det1");
    mover.setPropertyValue("X", "10");
    mover.setPropertyValue("Y", "20");
    mover.setPropertyValue("Z", "30");
    mover.setPropertyValue("RelativePosition", "0");
    mover.execute();

    Instrument_const_sptr inst = WS->getInstrument();
    // get pointer to the first detector in the bank
    boost::shared_ptr<const IComponent> comp =
        (*boost::dynamic_pointer_cast<const ICompAssembly>(
            (*boost::dynamic_pointer_cast<const ICompAssembly>(inst))[0]))[0];

    V3D pos = comp->getPos();
    TS_ASSERT_EQUALS(pos, V3D(10, 20, 30))
  }

private:
  std::string wsName;
  Detector *det1;
  boost::shared_ptr<Instrument> instrument;
  MatrixWorkspace_sptr WS;
};

#endif /*MOVEINSTRUMENTCOMPONENTTEST_H_*/
