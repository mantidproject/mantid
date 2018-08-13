#ifndef TUBEHELPERS_TEST_H_
#define TUBEHELPERS_TEST_H_

//---------------------------
// Includes
//---------------------------

#include "MantidGeometry/Objects/IObject.h"
#include "MantidNexusGeometry/NexusShapeFactory.h"
#include "MantidNexusGeometry/TubeHelpers.h"
#include "MantidTestHelpers/NexusGeometryTestHelpers.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace NexusGeometry;
using namespace NexusGeometryTestHelpers;

class TubeHelpersTest : public CxxTest::TestSuite {
public:
  void test_findTubesInvalid() {
    auto pixels = generateValidPixels();
    auto shape = createShape();
    auto detIds = getFakeDetIDs();

    auto tubes = TubeHelpers::findTubes(*shape, pixels, detIds);

    TS_ASSERT_EQUALS(tubes.size(), 2);
    TS_ASSERT(tubes[0].size() == tubes[1].size());
    TS_ASSERT_EQUALS(tubes[0].size(), 2);
  }

  void test_findTubesValid() {
    auto pixels = generateInvalidPixels();
    auto shape = createShape();
    auto detIds = getFakeDetIDs();
    auto tubes = TubeHelpers::findTubes(*shape, pixels, detIds);

    TS_ASSERT_EQUALS(tubes.size(), 0)
  }

  void test_findTubesMixed() {
    auto pixels = generateValidPixels();
    auto shape = createShape();
    auto detIds = getFakeDetIDs();

    // replace with invalid coord
    pixels.col(3) = Eigen::Vector3d(-0.7, -0.7, 0);

    auto tubes = TubeHelpers::findTubes(*shape, pixels, detIds);
    TS_ASSERT_EQUALS(tubes.size(), 1);
    TS_ASSERT_EQUALS(tubes[0].size(), 2);
  }
};

#endif // TUBEHELPERS_TEST_H_
