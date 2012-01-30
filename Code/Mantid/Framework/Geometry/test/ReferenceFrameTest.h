#ifndef MANTID_GEOMETRY_REFERENCEFRAMETEST_H_
#define MANTID_GEOMETRY_REFERENCEFRAMETEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>

#include "MantidGeometry/Instrument/ReferenceFrame.h"

using namespace Mantid;
using namespace Mantid::Geometry;

class ReferenceFrameTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ReferenceFrameTest *createSuite() { return new ReferenceFrameTest(); }
  static void destroySuite( ReferenceFrameTest *suite ) { delete suite; }

  void testDefaultSettings()
  {
    ReferenceFrame defaultInstance;
    TSM_ASSERT_EQUALS("Anticipated default from default constructor.", Y, defaultInstance.pointingUp());
    TSM_ASSERT_EQUALS("Anticipated default from default constructor.", Z, defaultInstance.pointingAlongBeam());
    TSM_ASSERT_EQUALS("Anticipated default from default constructor.", Right, defaultInstance.getHandedness());
    TSM_ASSERT_EQUALS("Anticipated default from default constructor.", "source", defaultInstance.origin());
  }

  void testGetUp()
  {
    ReferenceFrame frame1(X, Y, Right, "source");
    ReferenceFrame frame2(Z, Y, Right, "source");
    TS_ASSERT_EQUALS(X, frame1.pointingUp());
    TS_ASSERT_EQUALS(Z, frame2.pointingUp());
  }

  void testGetAlongBeam()
  {
    ReferenceFrame frame1(X, Y, Right, "source");
    ReferenceFrame frame2(Z, X, Right, "source");
    TS_ASSERT_EQUALS(Y, frame1.pointingAlongBeam());
    TS_ASSERT_EQUALS(X, frame2.pointingAlongBeam());
  }

  void testGetHandedNess()
  {
    ReferenceFrame frameRight(X, Y, Right, "source");
    ReferenceFrame frameLeft(X, Y, Left, "source");
    TS_ASSERT_EQUALS(Right, frameRight.getHandedness());
    TS_ASSERT_EQUALS(Left, frameLeft.getHandedness());
  }

  void testGetOrigin()
  {
    ReferenceFrame frame(X, Y, Right, "source");
    TS_ASSERT_EQUALS("source", frame.origin());
  }

  void testIdenticalUpAndBeamDirectionsThrow()
  {
    TS_ASSERT_THROWS(ReferenceFrame(X, X, Right, "source"), std::invalid_argument);
  }


};


#endif /* MANTID_GEOMETRY_REFERENCEFRAMETEST_H_ */