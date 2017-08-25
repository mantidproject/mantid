#ifndef MANTID_ALGORITHMS_BINDETECTORSCANTEST_H_
#define MANTID_ALGORITHMS_BINDETECTORSCANTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/BinDetectorScan.h"

using Mantid::Algorithms::BinDetectorScan;

class BinDetectorScanTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static BinDetectorScanTest *createSuite() { return new BinDetectorScanTest(); }
  static void destroySuite( BinDetectorScanTest *suite ) { delete suite; }


  void test_Something()
  {
    TS_FAIL( "You forgot to write a test!");
  }


};


#endif /* MANTID_ALGORITHMS_BINDETECTORSCANTEST_H_ */