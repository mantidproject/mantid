#ifndef MANTID_DATAHANDLING_DATABLOCKTEST_H_
#define MANTID_DATAHANDLING_DATABLOCKTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/DataBlock.h"

using Mantid::DataHandling::DataBlock;

class DataBlockTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static DataBlockTest *createSuite() { return new DataBlockTest(); }
  static void destroySuite( DataBlockTest *suite ) { delete suite; }


  void test_Something()
  {
    TS_FAIL( "You forgot to write a test!");
  }


};


#endif /* MANTID_DATAHANDLING_DATABLOCKTEST_H_ */