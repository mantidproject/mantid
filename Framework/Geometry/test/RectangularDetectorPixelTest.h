// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_GEOMETRY_RECTANGULARDETECTORPIXELTEST_H_
#define MANTID_GEOMETRY_RECTANGULARDETECTORPIXELTEST_H_

#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include <cxxtest/TestSuite.h>

#include "MantidGeometry/Instrument/RectangularDetectorPixel.h"

using namespace Mantid;
using namespace Mantid::Geometry;

class RectangularDetectorPixelTest : public CxxTest::TestSuite {
public:
  /// This test properly requires a RectangularDetector. See
  /// RectangularDetectorTest.
  void test_nothing() {}
};

#endif /* MANTID_GEOMETRY_RECTANGULARDETECTORPIXELTEST_H_ */
