// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

#include "MantidQtWidgets/InstrumentView/PanelsSurface.h"

using namespace MantidQt::MantidWidgets;
using namespace testing;

namespace {

// Create a test ProjectionSurface with a concrete implementation.
class TestSurface : public PanelsSurface {
public:
  // Publicly expose 'getPickID' so it can be tested
  std::size_t getPickIDPublic(int x, int y) const { return this->getPickID(x, y); }
};

} // namespace

class ProjectionSurfaceTest : public CxxTest::TestSuite {
public:
  static ProjectionSurfaceTest *createSuite() { return new ProjectionSurfaceTest(); }
  static void destroySuite(ProjectionSurfaceTest *suite) { delete suite; }

  void test_getPickID_returns_minus_one_when_pick_image_is_null() {
    auto const surface = std::make_unique<TestSurface>();
    TS_ASSERT_EQUALS(-1, surface->getPickIDPublic(1, 1));
  }
};
