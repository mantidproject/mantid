#ifndef TUBE_TEST_H_
#define TUBE_TEST_H_

//---------------------------
// Includes
//---------------------------

#include "MantidGeometry/Objects/IObject.h"
#include "MantidGeometry/Rendering/GeometryHandler.h"
#include "MantidGeometry/Rendering/ShapeInfo.h"
#include "MantidNexusGeometry/TubeBuilder.h"
#include "MantidTestHelpers/NexusGeometryTestHelpers.h"
#include <cxxtest/TestSuite.h>
#include <numeric>

using namespace Mantid;
using namespace NexusGeometry;
using namespace NexusGeometryTestHelpers;

class TubeBuilderTest : public CxxTest::TestSuite {
public:
  void test_constructor() {
    auto shape = createShape();
    const auto &shapeInfo = shape->getGeometryHandler()->shapeInfo();
    detail::TubeBuilder tube(*shape, Eigen::Vector3d(2, 2, 3), 10);

    TS_ASSERT_EQUALS(tube.size(), 1);
    TS_ASSERT_EQUALS(tube.tubePosition(), Eigen::Vector3d(2 - 0.00101, 2, 3));
    TS_ASSERT_EQUALS(tube.tubeRadius(), shapeInfo.radius());
    // Height should just be shape height
    TS_ASSERT_EQUALS(tube.tubeHeight(), shapeInfo.height());
    TS_ASSERT_EQUALS(tube.detPositions()[0], Eigen::Vector3d(2, 2, 3));
    TS_ASSERT_EQUALS(tube.detIDs()[0], 10);

    auto tubeShape = tube.shape();
    const auto &tubeShapeInfo = tubeShape->getGeometryHandler()->shapeInfo();
    TS_ASSERT_EQUALS(tubeShapeInfo, shapeInfo);
  }

  void testAddColinear() {
    auto shape = createShape();
    auto shapeInfo = shape->getGeometryHandler()->shapeInfo();
    detail::TubeBuilder tube(*shape, Eigen::Vector3d(0.00202, 1, 0), 10);

    TS_ASSERT(tube.addDetectorIfCoLinear(Eigen::Vector3d(0.00404, 1, 0), 11));
    TS_ASSERT_EQUALS(tube.size(), 2);
    // The shape height is 0.00202. See createShape for details.
    TS_ASSERT_EQUALS(tube.tubeHeight(), 0.00404);
    TS_ASSERT_EQUALS(tube.detIDs(), (std::vector<int>{10, 11}));
    TS_ASSERT_EQUALS(tube.detPositions()[0], Eigen::Vector3d(0.00202, 1, 0));
    TS_ASSERT_EQUALS(tube.detPositions()[1], Eigen::Vector3d(0.00404, 1, 0));
    TS_ASSERT_EQUALS(tube.tubePosition(), Eigen::Vector3d(0.00101, 1, 0));
  }

  void testAddNonColinear() {
    auto shape = createShape();
    auto shapeInfo = shape->getGeometryHandler()->shapeInfo();
    detail::TubeBuilder tube(*shape, Eigen::Vector3d(0.00202, 1, 0), 10);

    TS_ASSERT(!tube.addDetectorIfCoLinear(Eigen::Vector3d(0, 2, 0), 11));
    TS_ASSERT_EQUALS(tube.size(), 1);
  }
};

#endif // TUBE_TEST_H_
