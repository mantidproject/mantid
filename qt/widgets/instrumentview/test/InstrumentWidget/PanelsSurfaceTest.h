// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

#include "MantidFrameworkTestHelpers/ComponentCreationHelper.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidKernel/V3D.h"
#include "MantidQtWidgets/InstrumentView/PanelsSurface.h"
#include "MockInstrumentActor.h"

using namespace testing;

class PanelsSurfaceHelper : public PanelsSurface {
public:
  PanelsSurfaceHelper(const IInstrumentActor *rootActor, const Mantid::Kernel::V3D &origin,
                      const Mantid::Kernel::V3D &axis, const QSize &widgetSize, const bool maintainAspectRatio)
      : PanelsSurface(rootActor, origin, axis, widgetSize, maintainAspectRatio) {}
  PanelsSurfaceHelper() : PanelsSurface() {}
  void setupAxes() { PanelsSurface::setupAxes(); };
  void addDetector(size_t detIndex, const Mantid::Kernel::V3D &refPos, int bankIndex,
                   const Mantid::Kernel::Quat &rotation) {
    return PanelsSurface::addDetector(detIndex, refPos, bankIndex, rotation);
  }
  Mantid::Kernel::Quat calcBankRotation(const Mantid::Kernel::V3D &detPos, Mantid::Kernel::V3D normal) {
    return PanelsSurface::calcBankRotation(detPos, normal);
  }
  boost::optional<size_t> processTubes(size_t rootIndex) { return PanelsSurface::processTubes(rootIndex); }
  std::vector<UnwrappedDetector> getUnwrappedDetectors() { return m_unwrappedDetectors; };
  void addFlatBank() {
    auto *info = new FlatBankInfo(this);
    m_flatBanks << info;
  }
};

class PanelsSurfaceTest : public CxxTest::TestSuite {
public:
  static PanelsSurfaceTest *createSuite() { return new PanelsSurfaceTest(); }
  static void destroySuite(PanelsSurfaceTest *suite) { delete suite; }

  std::unique_ptr<NiceMock<MockInstrumentActor>> createMockInstrumentActor(MatrixWorkspace_sptr ws, int ndetectors) {
    // Hard to mock DetectorInfo and ComponentInfo so use real objects from a real workspace
    auto &detInfo = ws->detectorInfo();
    auto &compInfo = ws->componentInfo();
    auto instrumentActor = std::make_unique<NiceMock<MockInstrumentActor>>();
    EXPECT_CALL(*instrumentActor, ndetectors()).WillRepeatedly(Return(ndetectors));
    EXPECT_CALL(*instrumentActor, detectorInfo()).WillRepeatedly(ReturnRef(detInfo));
    EXPECT_CALL(*instrumentActor, componentInfo()).WillRepeatedly(ReturnRef(compInfo));
    EXPECT_CALL(*instrumentActor, getInstrument()).WillRepeatedly(Return(ws->getInstrument()));
    return instrumentActor;
  }

  void test_addDetector() {
    const int NDETECTORS = 2;
    auto ws = WorkspaceCreationHelper::create2DWorkspace(NDETECTORS, 1);
    Mantid::Kernel::V3D samplePosition{0., 0., 0.};
    Mantid::Kernel::V3D sourcePosition{0., 0., -10.};
    WorkspaceCreationHelper::createInstrumentForWorkspaceWithDistances(ws, samplePosition, sourcePosition,
                                                                       {{0., 0.1, -5.0}, {0., 0.1, 5.0}});
    auto instrumentActor = createMockInstrumentActor(ws, NDETECTORS);
    PanelsSurfaceHelper surface;
    surface.setupAxes();
    surface.resetInstrumentActor(instrumentActor.get());
    Mantid::Kernel::Quat q(90, {0., 0., 1.});
    surface.addFlatBank();
    surface.addDetector(0, {0., 0., 0.}, 0, q);
    surface.addDetector(1, {0., 0., 0.}, 0, q);
    auto unwrapppedDet = surface.getUnwrappedDetectors();
    TS_ASSERT(unwrapppedDet.size() == 2);
    TS_ASSERT_EQUALS(unwrapppedDet[0].u, -0.1);
    TS_ASSERT_DELTA(unwrapppedDet[0].v, 0., 1E-08);
    TS_ASSERT_EQUALS(unwrapppedDet[1].u, 0.1);
    TS_ASSERT_DELTA(unwrapppedDet[1].v, 0., 1E-08);
  }

  void test_calcBankRotation() {
    PanelsSurfaceHelper surface;
    Mantid::Kernel::V3D detPosPositiveZ{1.0, 0., 1.0};
    Mantid::Kernel::V3D detPosNegativeZ{1.0, 0., -1.0};

    // general case where rotation constructed in two stages
    Mantid::Kernel::V3D normalPointingToPlusYNegativeZ{0., 1.0, -1.0};
    normalPointingToPlusYNegativeZ.normalize();
    auto quat = surface.calcBankRotation(detPosPositiveZ, normalPointingToPlusYNegativeZ);
    auto anglesVector = quat.getEulerAngles("XYZ");
    TS_ASSERT_DELTA(anglesVector[0], -45.0, 1E-06);
    Mantid::Kernel::V3D normalPointingToPlusYPositiveZ{0., 1.0, 1.0};
    normalPointingToPlusYPositiveZ.normalize();
    quat = surface.calcBankRotation(detPosNegativeZ, normalPointingToPlusYPositiveZ);
    anglesVector = quat.getEulerAngles("XYZ");
    TS_ASSERT_DELTA(anglesVector[0], 45.0, 1E-06);

    // special case where normal is initially pointing away from viewer and is flipped to point at viewer
    normalPointingToPlusYPositiveZ.normalize();
    quat = surface.calcBankRotation(detPosNegativeZ, normalPointingToPlusYNegativeZ);
    anglesVector = quat.getEulerAngles("XYZ");
    TS_ASSERT_DELTA(anglesVector[0], -45.0, 1E-06);

    // special case where construct quaternion from a single rotation
    Mantid::Kernel::V3D normalPointingUp{0., 1.0, 0.};
    quat = surface.calcBankRotation(detPosPositiveZ, normalPointingUp);
    anglesVector = quat.getEulerAngles("XYZ");
    TS_ASSERT_DELTA(anglesVector[0], -90.0, 1E-06);
    quat = surface.calcBankRotation(detPosNegativeZ, normalPointingUp);
    anglesVector = quat.getEulerAngles("XYZ");
    TS_ASSERT_DELTA(anglesVector[0], 90.0, 1E-06);
  }

  void test_ProcessTubes() {
    const int NTUBES = 2;
    const int NDETSPERTUBE = 2;
    Mantid::API::MatrixWorkspace_sptr ws = WorkspaceCreationHelper::create2DWorkspace(2, 1);
    auto instrument = ComponentCreationHelper::createCylInstrumentWithVerticalOffsetsSpecified(
        NTUBES, {0., 0.}, NDETSPERTUBE, 0., 1., 0., 1.);
    ws->setInstrument(instrument);
    auto instAct = createMockInstrumentActor(ws, 4);
    PanelsSurfaceHelper surface;
    surface.setupAxes();
    surface.resetInstrumentActor(instAct.get());
    /*     9
          /|\
         8 7 6 <= sixteen pack
          / \
         4    5 <== tubes
        / \  / \
       0  1 2   3 <= detectors
    */
    surface.processTubes(4);
    auto unwrapppedDet = surface.getUnwrappedDetectors();
    TS_ASSERT(unwrapppedDet.size() == NTUBES * NDETSPERTUBE);
  }
};
