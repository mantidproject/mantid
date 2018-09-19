#ifndef MANTID_GEOMETRY_QLABTEST_H_
#define MANTID_GEOMETRY_QLABTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidGeometry/MDGeometry/QLab.h"
#include "MantidKernel/MDUnit.h"

using Mantid::Geometry::QLab;
using namespace Mantid::Kernel;

class QLabTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static QLabTest *createSuite() { return new QLabTest(); }
  static void destroySuite(QLabTest *suite) { delete suite; }

  void test_name() {
    Mantid::Geometry::QLab frame;
    TS_ASSERT_EQUALS(QLab::QLabName, frame.name());
  }

  void test_canConvertTo_unit() {
    Mantid::Geometry::QLab frame;
    InverseAngstromsUnit unit;
    TSM_ASSERT("Same unit type as is used for QLab", frame.canConvertTo(unit));
  }

  void test_cannotConvertTo_unit() {
    Mantid::Geometry::QLab frame;
    ReciprocalLatticeUnit unit;
    TSM_ASSERT("Not same unit type as is used for QLab",
               !frame.canConvertTo(unit));
    TSM_ASSERT_EQUALS("The equivalent special coordinate system should be QLab",
                      frame.equivalientSpecialCoordinateSystem(),
                      Mantid::Kernel::SpecialCoordinateSystem::QLab);
  }
};

#endif /* MANTID_GEOMETRY_QLABTEST_H_ */
