#ifndef MANTID_GEOMETRY_GRIDDETECTORPIXELTEST_H_
#define MANTID_GEOMETRY_GRIDDETECTORPIXELTEST_H_

#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include <cxxtest/TestSuite.h>

#include "MantidGeometry/Instrument/GridDetectorPixel.h"

using namespace Mantid;
using namespace Mantid::Geometry;

class GridDetectorPixelTest : public CxxTest::TestSuite {
public:
  /// This test properly requires a GridDetector. See
  /// GridDetectorTest.
  void test_nothing() {}
};

#endif /* MANTID_GEOMETRY_GRIDDETECTORPIXELTEST_H_ */
