#ifndef MANTID_GEOMETRY_RECTANGULARDETECTORPIXELTEST_H_
#define MANTID_GEOMETRY_RECTANGULARDETECTORPIXELTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"

#include "MantidGeometry/Instrument/RectangularDetectorPixel.h"
#include "MantidGeometry/Objects/BoundingBox.h"

using namespace Mantid;
using namespace Mantid::Geometry;

class RectangularDetectorPixelTest : public CxxTest::TestSuite {
public:
  /// This test properly requires a RectangularDetector. See
  /// RectangularDetectorTest.
  void test_nothing() {}
};

#endif /* MANTID_GEOMETRY_RECTANGULARDETECTORPIXELTEST_H_ */
