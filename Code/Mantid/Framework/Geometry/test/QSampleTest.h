#ifndef MANTID_GEOMETRY_QSAMPLETEST_H_
#define MANTID_GEOMETRY_QSAMPLETEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/MDUnit.h"
#include "MantidKernel/UnitLabelTypes.h"
#include "MantidGeometry/MDGeometry/QSample.h"

using Mantid::Geometry::QSample;
using namespace Mantid::Kernel;

class QSampleTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static QSampleTest *createSuite() { return new QSampleTest(); }
  static void destroySuite( QSampleTest *suite ) { delete suite; }


  void test_name()
  {
    QSample frame;
    TS_ASSERT_EQUALS(QSample::QSampleName, frame.name());
  }

  void test_canConvertTo_unit(){
    QSample frame;
    InverseAngstromsUnit unit;
    TSM_ASSERT("Same unit type as is used for QLab", frame.canConvertTo(unit));
  }

  void test_cannotConvertTo_unit(){
    QSample frame;
    ReciprocalLatticeUnit unit;
    TSM_ASSERT("Not same unit type as is used for QLab", !frame.canConvertTo(unit));
  }



};


#endif /* MANTID_GEOMETRY_QSAMPLETEST_H_ */
