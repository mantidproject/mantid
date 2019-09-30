// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_TESTPARDETECTOR__
#define MANTID_TESTPARDETECTOR__

#include "MantidGeometry/Instrument/Component.h"
#include "MantidGeometry/Instrument/Detector.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid::Geometry;

class ParDetectorTest : public CxxTest::TestSuite {
public:
  void testNameConstructor() {
    Detector det("det1", 0, nullptr);

    ParameterMap_sptr pmap(new ParameterMap());
    boost::shared_ptr<Detector> pdet(det.cloneParameterized(pmap.get()));

    TS_ASSERT_EQUALS(pdet->getName(), "det1");
    TS_ASSERT(!pdet->getParent());
    TS_ASSERT_EQUALS(pdet->getID(), 0);
  }

  void testNameParentConstructor() {
    Component parent("Parent");
    Detector det("det1", 0, &parent);

    ParameterMap_sptr pmap(new ParameterMap());
    boost::shared_ptr<Detector> pdet(det.cloneParameterized(pmap.get()));

    TS_ASSERT_EQUALS(pdet->getName(), "det1");
    TS_ASSERT(pdet->getParent());
    TS_ASSERT_EQUALS(pdet->getID(), 0);
  }

  void testId() {
    int id1 = 41;
    Detector det("det1", id1, nullptr);

    ParameterMap_sptr pmap(new ParameterMap());
    boost::shared_ptr<Detector> pdet(det.cloneParameterized(pmap.get()));

    TS_ASSERT_EQUALS(pdet->getID(), id1);
  }

  void testType() {
    Detector det("det", 0, nullptr);

    ParameterMap_sptr pmap(new ParameterMap());
    boost::shared_ptr<Detector> pdet(det.cloneParameterized(pmap.get()));

    TS_ASSERT_EQUALS(pdet->type(), "DetectorComponent");
  }

  void testMasked() {
    Detector det("det", 0, nullptr);

    ParameterMap_sptr pmap(new ParameterMap());
    boost::shared_ptr<Detector> pdet(det.cloneParameterized(pmap.get()));

    // Reading and writing masking should throw: Masking is now stored in
    // DetectorInfo and ParameterMap should reject it.
    TS_ASSERT_THROWS(pmap->get(&det, "masked"), const std::runtime_error &);
    TS_ASSERT_THROWS(pmap->addBool(&det, "masked", true),
                     const std::runtime_error &);
  }

  void testGetNumberParameter() {
    Detector det("det", 0, nullptr);

    ParameterMap_sptr pmap(new ParameterMap());
    pmap->add("double", &det, "testparam", 5.0);
    boost::shared_ptr<Detector> pdet(det.cloneParameterized(pmap.get()));
    IDetector *idet = static_cast<IDetector *>(pdet.get());

    TS_ASSERT_EQUALS(idet->getNumberParameter("testparam").size(), 1);
    TS_ASSERT_DELTA(idet->getNumberParameter("testparam")[0], 5.0, 1e-08);
  }

  void testGetPositionParameter() {
    Detector det("det", 0, nullptr);

    ParameterMap_sptr pmap(new ParameterMap());
    pmap->add("V3D", &det, "testparam", Mantid::Kernel::V3D(0.5, 1.0, 1.5));
    boost::shared_ptr<Detector> pdet(det.cloneParameterized(pmap.get()));
    IDetector *idet = static_cast<IDetector *>(pdet.get());

    std::vector<Mantid::Kernel::V3D> pos =
        idet->getPositionParameter("testparam");

    TS_ASSERT_EQUALS(pos.size(), 1);
    TS_ASSERT_DELTA(pos[0].X(), 0.5, 1e-08);
    TS_ASSERT_DELTA(pos[0].Y(), 1.0, 1e-08);
    TS_ASSERT_DELTA(pos[0].Z(), 1.5, 1e-08);
  }

  void testGetRotationParameter() {
    Detector det("det", 0, nullptr);

    ParameterMap_sptr pmap(new ParameterMap());
    pmap->add("Quat", &det, "testparam",
              Mantid::Kernel::Quat(1.0, 0.25, 0.5, 0.75));
    boost::shared_ptr<Detector> pdet(det.cloneParameterized(pmap.get()));
    IDetector *idet = static_cast<IDetector *>(pdet.get());

    std::vector<Mantid::Kernel::Quat> rot =
        idet->getRotationParameter("testparam");

    TS_ASSERT_EQUALS(rot.size(), 1);
    TS_ASSERT_DELTA(rot[0].real(), 1.0, 1e-08);
    TS_ASSERT_DELTA(rot[0].imagI(), 0.25, 1e-08);
    TS_ASSERT_DELTA(rot[0].imagJ(), 0.5, 1e-08);
    TS_ASSERT_DELTA(rot[0].imagK(), 0.75, 1e-08);
  }
};

#endif
