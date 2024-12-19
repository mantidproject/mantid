// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

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
