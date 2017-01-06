#ifndef TESTDETECTORGROUP_H_
#define TESTDETECTORGROUP_H_

#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/Instrument/DetectorGroup.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include <boost/make_shared.hpp>
#include <cxxtest/TestSuite.h>

using namespace Mantid::Geometry;
using namespace Mantid;
using Mantid::Kernel::V3D;
using Mantid::Kernel::Quat;

class DetectorGroupTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static DetectorGroupTest *createSuite() { return new DetectorGroupTest(); }
  static void destroySuite(DetectorGroupTest *suite) { delete suite; }

  DetectorGroupTest() : m_origin() {
    m_detGroup =
        ComponentCreationHelper::createDetectorGroupWith5CylindricalDetectors();
  }

  void testConstructors() {
    TS_ASSERT_EQUALS(m_detGroup->getDetectorIDs().size(), 5);
  }

  void testGetPos() {
    V3D pos;
    TS_ASSERT_THROWS_NOTHING(pos = m_detGroup->getPos());
    TS_ASSERT_DELTA(pos.X(), 3.0, 1e-08);
    TS_ASSERT_DELTA(pos.Y(), 2.0, 1e-08);
    TS_ASSERT_DELTA(pos.Z(), 2.0, 1e-08);
  }

  void testGetDetectorIDs() {
    std::vector<detid_t> detIDs = m_detGroup->getDetectorIDs();
    TS_ASSERT_EQUALS(detIDs.size(), 5);
    for (size_t i = 0; i < detIDs.size(); ++i) {
      TS_ASSERT_EQUALS(detIDs[i], i + 1);
    }
  }

  void testGetDetectors() {
    std::vector<IDetector_const_sptr> dets = m_detGroup->getDetectors();
    TS_ASSERT_EQUALS(dets.size(), 5);
    for (size_t i = 0; i < dets.size(); ++i) {
      TS_ASSERT(dets[i]);
    }
  }

  void testIdentRectShape() {
    Kernel::V3D center;
    boost::shared_ptr<Mantid::Geometry::DetectorGroup> rectGroup =
        ComponentCreationHelper::createDetectorGroupWith5CylindricalDetectors();
    TSM_ASSERT_EQUALS("should be rectangular shape", rect,
                      rectGroup->getTopology(center));
  }
  void testIdentRectShapeWithGaps() {
    Kernel::V3D center;
    boost::shared_ptr<Mantid::Geometry::DetectorGroup> rectGroup =
        ComponentCreationHelper::
            createDetectorGroupWithNCylindricalDetectorsWithGaps(4, 0.0);
    TSM_ASSERT_EQUALS("should be rectangular shape", rect,
                      rectGroup->getTopology(center));
  }
  void testIdentRingShape() {
    Kernel::V3D center;
    boost::shared_ptr<Mantid::Geometry::DetectorGroup> rectGroup =
        ComponentCreationHelper::createRingOfCylindricalDetectors();
    TSM_ASSERT_EQUALS("should be ring shape", cyl,
                      rectGroup->getTopology(center));
  }
  void testGetID() { TS_ASSERT_EQUALS(m_detGroup->getID(), 1); }

  void testGetDistance() {
    TS_ASSERT_DELTA(m_detGroup->getDistance(m_origin), 4.24614987, 1e-08);
  }

  void testBoundingBox() {}

  void testAddDetector() {
    boost::shared_ptr<DetectorGroup> detg =
        ComponentCreationHelper::createDetectorGroupWith5CylindricalDetectors();
    auto d = boost::make_shared<Detector>("d", 6, nullptr);
    d->setPos(6.0, 3.0, 2.0);
    detg->addDetector(d);
    TS_ASSERT_EQUALS(detg->getID(), 1);
    TS_ASSERT_DELTA(detg->getPos()[0], 3.5, 1e-08);
    TS_ASSERT_DELTA(detg->getPos()[1], 2.16666667, 1e-08);
    TS_ASSERT_DELTA(detg->getPos()[2], 2.0, 1e-08);
  }

  void test_That_The_Bounding_Box_Is_Large_Enough_For_All_Of_The_Detectors() {
    BoundingBox bbox;
    m_detGroup->getBoundingBox(bbox);
    V3D min = bbox.minPoint();
    V3D max = bbox.maxPoint();
    TS_ASSERT_DELTA(min.X(), 0.5, 1e-08);
    TS_ASSERT_DELTA(min.Y(), 2.0, 1e-08);
    TS_ASSERT_DELTA(min.Z(), 1.5, 1e-08);
    TS_ASSERT_DELTA(max.X(), 5.5, 1e-08);
    TS_ASSERT_DELTA(max.Y(), 3.5, 1e-08);
    TS_ASSERT_DELTA(max.Z(), 2.5, 1e-08);
  }

  void test_signed_two_theta() {
    // Create a cluster consisting of a single detector.
    DetectorGroup cluster;
    Detector *det = new Detector("det", 0, 0);
    det->setPos(1, -1, 0);
    cluster.addDetector(IDetector_const_sptr(det));

    V3D axis(1, 0, 0);
    V3D sample(0, 0, 0);
    V3D up(0, 0, 1);

    double theta = cluster.getTwoTheta(sample, axis);
    double signedThetaCluster = cluster.getSignedTwoTheta(sample, axis, up);
    double signedThetaDetector = det->getSignedTwoTheta(sample, axis, up);

    TS_ASSERT_EQUALS(theta, std::abs(signedThetaCluster));
    TSM_ASSERT_EQUALS("Should be same as for individual detector",
                      signedThetaCluster, signedThetaDetector);
  }

private:
  boost::shared_ptr<DetectorGroup> m_detGroup;
  Component m_origin;
};

#endif /*TESTDETECTORGROUP_H_*/
