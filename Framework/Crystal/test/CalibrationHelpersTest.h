#ifndef MANTID_CRYSTAL_CALIBRATIONHELPERSTEST_H_
#define MANTID_CRYSTAL_CALIBRATIONHELPERSTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCrystal/CalibrationHelpers.h"

using Mantid::Crystal::CalibrationHelpers;

class CalibrationHelpersTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CalibrationHelpersTest *createSuite() { return new CalibrationHelpersTest(); }
  static void destroySuite( CalibrationHelpersTest *suite ) { delete suite; }


  void test_Something()
  {
    TS_FAIL( "You forgot to write a test!");
  }


};


#endif /* MANTID_CRYSTAL_CALIBRATIONHELPERSTEST_H_ */