#ifndef MANTID_CURVEFITTING_CRYSTALFIELDPEAKSTEST_H_
#define MANTID_CURVEFITTING_CRYSTALFIELDPEAKSTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/Functions/CrystalFieldPeaks.h"

using Mantid::CurveFitting::Functions::CrystalFieldPeaks;

class CrystalFieldPeaksTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CrystalFieldPeaksTest *createSuite() { return new CrystalFieldPeaksTest(); }
  static void destroySuite( CrystalFieldPeaksTest *suite ) { delete suite; }


  void test_Something()
  {
    TS_FAIL( "You forgot to write a test!");
  }


};


#endif /* MANTID_CURVEFITTING_CRYSTALFIELDPEAKSTEST_H_ */