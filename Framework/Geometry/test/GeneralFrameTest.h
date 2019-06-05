// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_GEOMETRY_GENERALFRAMETEST_H_
#define MANTID_GEOMETRY_GENERALFRAMETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidGeometry/MDGeometry/GeneralFrame.h"
#include "MantidKernel/UnitLabel.h"
#include "MantidKernel/UnitLabelTypes.h"

using Mantid::Geometry::GeneralFrame;
using namespace Mantid::Kernel;

class GeneralFrameTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static GeneralFrameTest *createSuite() { return new GeneralFrameTest(); }
  static void destroySuite(GeneralFrameTest *suite) { delete suite; }

  void test_string_construction() {
    GeneralFrame frame("Temperature", "DegC");
    TS_ASSERT_EQUALS("Temperature", frame.name());
    TS_ASSERT_EQUALS(UnitLabel("DegC"), frame.getUnitLabel());
    TSM_ASSERT_EQUALS("The equivalent special coordinate system should be None",
                      frame.equivalientSpecialCoordinateSystem(),
                      Mantid::Kernel::SpecialCoordinateSystem::None);
  }

  void test_string_unit_construction() {
    auto unit = std::make_unique<LabelUnit>(Units::Symbol::Metre);
    GeneralFrame frame("Distance", std::move(unit) /*sink transfer ownership*/);
    TS_ASSERT(unit.get() == nullptr);
    TS_ASSERT_EQUALS(Units::Symbol::Metre, frame.getUnitLabel());
    TS_ASSERT_EQUALS("Distance", frame.name());
  }
};

#endif /* MANTID_GEOMETRY_GENERALFRAMETEST_H_ */
