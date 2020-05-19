#pragma once
#include "MantidGeometry/MDGeometry/MDTypes.h"
#include "MantidMDAlgorithms/MDBoxMaskFunction.h"

#include <cxxtest/TestSuite.h>
#include <memory>

using Mantid::Geometry::MDAlgorithms::MDBoxMaskFunction;
using Mantid::Kernel::V3D;

class MDBoxMaskFunctionTest : public CxxTest::TestSuite {
public:
  //-------------------------------------------------------------------------------
  void test_exec() {

    auto function = std::make_unique<MDBoxMaskFunction>(V3D(0., 0., 0.), 1.0);

    Mantid::coord_t pt[3];
    // point 1
    pt[0] = 2.0;
    pt[1] = 2.0;
    pt[2] = 2.0;
    TS_ASSERT_EQUALS(function->isPointContained(pt), false);
    // point 2
    pt[0] = 0.5;
    pt[1] = 0.0;
    pt[2] = 0.0;
    TS_ASSERT_EQUALS(function->isPointContained(pt), true);
  }
};
