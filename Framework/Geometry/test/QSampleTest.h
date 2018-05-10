#ifndef MANTID_GEOMETRY_QSAMPLETEST_H_
#define MANTID_GEOMETRY_QSAMPLETEST_H_

#include "MantidGeometry/MDGeometry/QSample.h"
#include "MantidKernel/MDUnit.h"
#include "MantidKernel/UnitLabelTypes.h"
#include <cxxtest/TestSuite.h>

using Mantid::Geometry::QSample;
using namespace Mantid::Kernel;

class QSampleTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static QSampleTest *createSuite() { return new QSampleTest(); }
  static void destroySuite(QSampleTest *suite) { delete suite; }

  void test_name() {
    Mantid::Geometry::QSample frame;
    TS_ASSERT_EQUALS(QSample::QSampleName, frame.name());
  }

  void test_canConvertTo_unit() {
    Mantid::Geometry::QSample frame;
    InverseAngstromsUnit unit;
    TSM_ASSERT("Same unit type as is used for QLab", frame.canConvertTo(unit));
  }

  void test_cannotConvertTo_unit() {
    Mantid::Geometry::QSample frame;
    ReciprocalLatticeUnit unit;
    TSM_ASSERT("Not same unit type as is used for QLab",
               !frame.canConvertTo(unit));
    TSM_ASSERT_EQUALS(
        "The equivalent special coordinate system should be QSample",
        frame.equivalientSpecialCoordinateSystem(),
        Mantid::Kernel::SpecialCoordinateSystem::QSample);
  }
};

#endif /* MANTID_GEOMETRY_QSAMPLETEST_H_ */
