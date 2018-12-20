#ifndef TUBEHELPERS_TEST_H_
#define TUBEHELPERS_TEST_H_

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
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static TubeHelpersTest *createSuite() { return new TubeHelpersTest(); }
  static void destroySuite(TubeHelpersTest *suite) { delete suite; }

  void test_CoLinearDetectorsProduceTubes() {
    auto pixels = generateCoLinearPixels();
    auto shape = createShape();
    auto detIds = getFakeDetIDs();

    // Inputs represent two parallel tubes comprising two cylindrical detectors
    // each.
    auto tubes = TubeHelpers::findAndSortTubes(*shape, pixels, detIds);

    TS_ASSERT_EQUALS(tubes.size(), 2);
    TS_ASSERT(tubes[0].size() == tubes[1].size());
    TS_ASSERT_EQUALS(tubes[0].size(), 2);
  }

  void test_NonColinearDetectorsDoNotProduceTubes() {
    auto pixels = generateNonCoLinearPixels();
    auto shape = createShape();
    auto detIds = getFakeDetIDs();

    // Inputs represent 4 discrete cylinders which are not coLinear.
    auto tubes = TubeHelpers::findAndSortTubes(*shape, pixels, detIds);

    TS_ASSERT_EQUALS(tubes.size(), 0)
  }

  void test_MixtureOfCoLinearAndNonCoLinearTubes() {
    auto pixels = generateCoLinearPixels();
    auto shape = createShape();
    auto detIds = getFakeDetIDs();

    // replace with coord which is not CoLinear and thus will not be part of any
    // tube
    pixels.col(3) = Eigen::Vector3d(-0.7, -0.7, 0);

    // Inputs represent one tube comprising of two cylinders and two discrete
    // non-colinear detectors.
    auto tubes = TubeHelpers::findAndSortTubes(*shape, pixels, detIds);
    TS_ASSERT_EQUALS(tubes.size(), 1);
    TS_ASSERT_EQUALS(tubes[0].size(), 2);
  }
};

#endif // TUBEHELPERS_TEST_H_
