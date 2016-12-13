#ifndef MANTID_BEAMLINE_DETECTORINFOTEST_H_
#define MANTID_BEAMLINE_DETECTORINFOTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidBeamline/DetectorInfo.h"

using Mantid::Beamline::DetectorInfo;

class DetectorInfoTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static DetectorInfoTest *createSuite() { return new DetectorInfoTest(); }
  static void destroySuite( DetectorInfoTest *suite ) { delete suite; }


  void test_Something()
  {
    TS_FAIL( "You forgot to write a test!");
  }


};


#endif /* MANTID_BEAMLINE_DETECTORINFOTEST_H_ */