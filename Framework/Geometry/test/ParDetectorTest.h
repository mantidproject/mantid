// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/Component.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/Instrument/ParameterMap.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid::Geometry;

class ParDetectorTest : public CxxTest::TestSuite {
public:
  void testNameConstructor() {
    Detector det("det1", 0, nullptr);

    ParameterMap_sptr pmap(new ParameterMap());
    std::shared_ptr<Detector> pdet(det.cloneParameterized(pmap.get()));

    TS_ASSERT_EQUALS(pdet->getName(), "det1");
    TS_ASSERT(!pdet->getParent());
    TS_ASSERT_EQUALS(pdet->getID(), 0);
  }

  void testNameParentConstructor() {
    Component parent("Parent");
    Detector det("det1", 0, &parent);

    ParameterMap_sptr pmap(new ParameterMap());
    std::shared_ptr<Detector> pdet(det.cloneParameterized(pmap.get()));

    TS_ASSERT_EQUALS(pdet->getName(), "det1");
    TS_ASSERT(pdet->getParent());
    TS_ASSERT_EQUALS(pdet->getID(), 0);
  }

  void testId() {
    int id1 = 41;
    Detector det("det1", id1, nullptr);

    ParameterMap_sptr pmap(new ParameterMap());
    std::shared_ptr<Detector> pdet(det.cloneParameterized(pmap.get()));

    TS_ASSERT_EQUALS(pdet->getID(), id1);
  }

  void testType() {
    Detector det("det", 0, nullptr);

    ParameterMap_sptr pmap(new ParameterMap());
    std::shared_ptr<Detector> pdet(det.cloneParameterized(pmap.get()));

    TS_ASSERT_EQUALS(pdet->type(), "DetectorComponent");
  }

  void testMasked() {
    auto const [instrument, det, pmap] = attachedDetector();
    std::shared_ptr<Detector> pdet(det->cloneParameterized(pmap.get()));

    // Reading and writing masking should throw: Masking is now stored in
    // DetectorInfo and ParameterMap should reject it.
    TS_ASSERT_THROWS(pmap->get(det, "masked"), const std::runtime_error &);
    TS_ASSERT_THROWS(pmap->addBool(det, "masked", true), const std::runtime_error &);
  }

  void testGetNumberParameter() {
    auto const [instrument, det, pmap] = attachedDetector();
    auto &componentInfo = pmap->mutableComponentInfo();
    componentInfo.addDouble(det->index(), "testparam", 5.0);
    std::shared_ptr<Detector> pdet(det->cloneParameterized(pmap.get()));
    IDetector *idet = static_cast<IDetector *>(pdet.get());

    TS_ASSERT_EQUALS(idet->getNumberParameter("testparam").size(), 1);
    TS_ASSERT_DELTA(idet->getNumberParameter("testparam")[0], 5.0, 1e-08);
  }

  void testGetPositionParameter() {
    auto const [instrument, det, pmap] = attachedDetector();
    auto &componentInfo = pmap->mutableComponentInfo();
    componentInfo.addV3D(det->index(), "testparam", Mantid::Kernel::V3D(0.5, 1.0, 1.5));
    std::shared_ptr<Detector> pdet(det->cloneParameterized(pmap.get()));
    IDetector *idet = static_cast<IDetector *>(pdet.get());

    std::vector<Mantid::Kernel::V3D> pos = idet->getPositionParameter("testparam");

    TS_ASSERT_EQUALS(pos.size(), 1);
    TS_ASSERT_DELTA(pos[0].X(), 0.5, 1e-08);
    TS_ASSERT_DELTA(pos[0].Y(), 1.0, 1e-08);
    TS_ASSERT_DELTA(pos[0].Z(), 1.5, 1e-08);
  }

  void testGetRotationParameter() {
    auto const [instrument, det, pmap] = attachedDetector();
    auto &componentInfo = pmap->mutableComponentInfo();
    componentInfo.addQuat(det->index(), "testparam", Mantid::Kernel::Quat(1.0, 0.25, 0.5, 0.75));
    std::shared_ptr<Detector> pdet(det->cloneParameterized(pmap.get()));
    IDetector *idet = static_cast<IDetector *>(pdet.get());

    std::vector<Mantid::Kernel::Quat> rot = idet->getRotationParameter("testparam");

    TS_ASSERT_EQUALS(rot.size(), 1);
    TS_ASSERT_DELTA(rot[0].real(), 1.0, 1e-08);
    TS_ASSERT_DELTA(rot[0].imagI(), 0.25, 1e-08);
    TS_ASSERT_DELTA(rot[0].imagJ(), 0.5, 1e-08);
    TS_ASSERT_DELTA(rot[0].imagK(), 0.75, 1e-08);
  }

private:
  struct AttachedDetector {
    std::shared_ptr<Instrument> instrument;
    Detector *det;
    ParameterMap_sptr pmap;
  };
  /// An instrument holding a single detector, which it owns, with a map attached to it
  static AttachedDetector attachedDetector() {
    auto instrument = std::make_shared<Instrument>("Instrument");
    auto *const det = new Detector("det", 0, nullptr);
    instrument->add(det);
    instrument->markAsDetector(det);
    auto pmap = std::make_shared<ParameterMap>();
    pmap->setInstrument(instrument.get());
    return {instrument, det, pmap};
  }
};
