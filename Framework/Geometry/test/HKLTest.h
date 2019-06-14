// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_GEOMETRY_HKLTEST_H_
#define MANTID_GEOMETRY_HKLTEST_H_

#include "MantidGeometry/MDGeometry/HKL.h"
#include "MantidKernel/MDUnit.h"

#include <cxxtest/TestSuite.h>
#include <memory>

using Mantid::Geometry::HKL;

class HKLTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static HKLTest *createSuite() { return new HKLTest(); }
  static void destroySuite(HKLTest *suite) { delete suite; }

  void test_check_unit_compatibility() {

    TSM_ASSERT_THROWS("Input unit for this frame must be a QUnit",
                      HKL(new Mantid::Kernel::LabelUnit("MeV")),
                      std::invalid_argument &);
  }

  void test_check_unit_compatibility_unique_ptr() {
    std::unique_ptr<Mantid::Kernel::MDUnit> badUnit =
        std::make_unique<Mantid::Kernel::LabelUnit>("MeV");

    HKL *testHKL = nullptr;
    TSM_ASSERT_THROWS("Input unit for this frame must be a QUnit",
                      testHKL = new HKL(badUnit), std::invalid_argument &);
    TSM_ASSERT("Construction should not have succeeded", testHKL == nullptr);
    TSM_ASSERT("Ownership of input should not have changed",
               badUnit.get() != nullptr);
  }

  void test_name() {
    HKL frame(new Mantid::Kernel::ReciprocalLatticeUnit);
    TS_ASSERT_EQUALS(HKL::HKLName, frame.name());
    TSM_ASSERT_EQUALS("The equivalent special coordinate system should be HKL",
                      frame.equivalientSpecialCoordinateSystem(),
                      Mantid::Kernel::SpecialCoordinateSystem::HKL);
  }
};

#endif /* MANTID_GEOMETRY_HKLTEST_H_ */
