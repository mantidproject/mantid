#ifndef MANTID_DATAHANDLING_ROTATESOURCETEST_H_
#define MANTID_DATAHANDLING_ROTATESOURCETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/Component.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidKernel/V3D.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;

class RotateSourceTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static RotateSourceTest *createSuite() { return new RotateSourceTest(); }
  static void destroySuite(RotateSourceTest *suite) { delete suite; }

  void testInit() {
    IAlgorithm_sptr alg = AlgorithmManager::Instance().create("RotateSource");
    alg->initialize();
    TS_ASSERT(alg->isInitialized())
  }

  void testRotateClockwise() {

    // The instrument
    Instrument_sptr instr = boost::make_shared<Instrument>();
    instr->setReferenceFrame(
        boost::make_shared<ReferenceFrame>(Y, Z, Left, ""));

    // The source
    ObjComponent *source = new ObjComponent("source");
    source->setPos(V3D(0, 0, 1));
    instr->add(source);
    instr->markAsSource(source);

    // The sample
    ObjComponent *sample = new ObjComponent("sample");
    sample->setPos(V3D(0, 0, 0));
    instr->add(sample);
    instr->markAsSamplePos(sample);

    // The workspace
    auto ws = WorkspaceCreationHelper::create2DWorkspace123(1, 1);
    ws->setInstrument(instr);
    // The angle
    double theta = 90.;

    IAlgorithm_sptr alg = AlgorithmManager::Instance().create("RotateSource");
    alg->setChild(true);
    alg->setProperty("Workspace", ws);
    alg->setProperty("Angle", theta);

    TS_ASSERT_THROWS_NOTHING(alg->execute());

    auto newPos = ws->getInstrument()->getSource()->getPos();

    TS_ASSERT_DELTA(newPos.X(), 0.0, 1E-5);
    TS_ASSERT_DELTA(newPos.Y(), 1.0, 1E-5);
    TS_ASSERT_DELTA(newPos.Z(), 0.0, 1E-5);
  }

  void testRotateCounterClockwise() {

    // The instrument
    Instrument_sptr instr = boost::make_shared<Instrument>();
    instr->setReferenceFrame(
        boost::make_shared<ReferenceFrame>(Y, Z, Left, ""));

    // The source
    ObjComponent *source = new ObjComponent("source");
    source->setPos(V3D(0, 0, 1));
    instr->add(source);
    instr->markAsSource(source);

    // The sample
    ObjComponent *sample = new ObjComponent("sample");
    sample->setPos(V3D(0, 0, 0));
    instr->add(sample);
    instr->markAsSamplePos(sample);

    // The workspace
    auto ws = WorkspaceCreationHelper::create2DWorkspace123(1, 1);
    ws->setInstrument(instr);
    // The angle
    double theta = -90.;

    IAlgorithm_sptr alg = AlgorithmManager::Instance().create("RotateSource");
    alg->setChild(true);
    alg->setProperty("Workspace", ws);
    alg->setProperty("Angle", theta);

    TS_ASSERT_THROWS_NOTHING(alg->execute());

    auto newPos = ws->getInstrument()->getSource()->getPos();

    TS_ASSERT_DELTA(newPos.X(), 0.0, 1E-5);
    TS_ASSERT_DELTA(newPos.Y(), -1.0, 1E-5);
    TS_ASSERT_DELTA(newPos.Z(), 0.0, 1E-5);
  }

  void testRotateClockwiseSampleAt001() {

    // The instrument
    Instrument_sptr instr = boost::make_shared<Instrument>();
    instr->setReferenceFrame(
        boost::make_shared<ReferenceFrame>(Y, Z, Right, ""));

    // The source
    ObjComponent *source = new ObjComponent("source");
    source->setPos(V3D(0, 0, 2));
    instr->add(source);
    instr->markAsSource(source);

    // The sample
    ObjComponent *sample = new ObjComponent("sample");
    sample->setPos(V3D(0, 0, 1));
    instr->add(sample);
    instr->markAsSamplePos(sample);

    // The workspace
    auto ws = WorkspaceCreationHelper::create2DWorkspace123(1, 1);
    ws->setInstrument(instr);
    // The angle
    double theta = 90.;

    IAlgorithm_sptr alg = AlgorithmManager::Instance().create("RotateSource");
    alg->setChild(true);
    alg->setProperty("Workspace", ws);
    alg->setProperty("Angle", theta);

    TS_ASSERT_THROWS_NOTHING(alg->execute());

    auto newPos = ws->getInstrument()->getSource()->getPos();

    TS_ASSERT_DELTA(newPos.X(), 0.0, 1E-5);
    TS_ASSERT_DELTA(newPos.Y(), -1.0, 1E-5);
    TS_ASSERT_DELTA(newPos.Z(), 1.0, 1E-5);
  }

  void testRotateClockwiseSampleAt111() {

    // The instrument
    Instrument_sptr instr = boost::make_shared<Instrument>();
    instr->setReferenceFrame(
        boost::make_shared<ReferenceFrame>(Y, Z, Right, ""));

    // The source
    ObjComponent *source = new ObjComponent("source");
    source->setPos(V3D(1, 1, 2));
    instr->add(source);
    instr->markAsSource(source);

    // The sample
    ObjComponent *sample = new ObjComponent("sample");
    sample->setPos(V3D(1, 1, 1));
    instr->add(sample);
    instr->markAsSamplePos(sample);

    // The workspace
    auto ws = WorkspaceCreationHelper::create2DWorkspace123(1, 1);
    ws->setInstrument(instr);
    // The angle
    double theta = 90.;

    IAlgorithm_sptr alg = AlgorithmManager::Instance().create("RotateSource");
    alg->setChild(true);
    alg->setProperty("Workspace", ws);
    alg->setProperty("Angle", theta);

    TS_ASSERT_THROWS_NOTHING(alg->execute());

    auto newPos = ws->getInstrument()->getSource()->getPos();

    TS_ASSERT_DELTA(newPos.X(), 1.0, 1E-5);
    TS_ASSERT_DELTA(newPos.Y(), 0.0, 1E-5);
    TS_ASSERT_DELTA(newPos.Z(), 1.0, 1E-5);
  }
};

#endif /* MANTID_DATAHANDLING_ROTATESOURCETEST_H_ */
