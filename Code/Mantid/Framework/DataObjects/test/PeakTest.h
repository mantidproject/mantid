#ifndef MANTID_DATAOBJECTS_PEAKTEST_H_
#define MANTID_DATAOBJECTS_PEAKTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>

#include "MantidDataObjects/Peak.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"

using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;

class PeakTest : public CxxTest::TestSuite
{
public:
  /// Common instrument
  IInstrument_sptr inst;
  void setUp()
  {
    inst = ComponentCreationHelper::createTestInstrumentRectangular(5, 100);
  }

  void test_constructor()
  {
    // detector IDs start at 10000
    Peak p(inst, 10000, 2.0);
    TS_ASSERT_DELTA(p.getInitialEnergy(), 2.0, 1e-5)
    TS_ASSERT_DELTA(p.getFinalEnergy(), 2.0, 1e-5)
    TS_ASSERT_DELTA(p.getH(), 0.0, 1e-5)
    TS_ASSERT_DELTA(p.getK(), 0.0, 1e-5)
    TS_ASSERT_DELTA(p.getL(), 0.0, 1e-5)
    TS_ASSERT_EQUALS(p.getDetectorID(), 10000)
  }

  void test_badDetectorID_throws()
  {
    Peak p(inst, 10000, 2.0);
    TS_ASSERT_THROWS_ANYTHING( p.setDetectorID(7) );
  }

  void test_getBank_and_row()
  {
    Peak p(inst, 10000, 2.0);
    TS_ASSERT_EQUALS(p.getBankName(), "bank1")
    TS_ASSERT_EQUALS(p.getRow(), 0)
    TS_ASSERT_EQUALS(p.getCol(), 0)
  }


};


#endif /* MANTID_DATAOBJECTS_PEAKTEST_H_ */

