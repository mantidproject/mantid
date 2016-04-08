#ifndef MANTID_DATAHANDLING_SAVENXCANSASTEST_H_
#define MANTID_DATAHANDLING_SAVENXCANSASTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/SaveNXcanSAS.h"

using Mantid::DataHandling::SaveNXcanSAS;

class SaveNXcanSASTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SaveNXcanSASTest *createSuite() { return new SaveNXcanSASTest(); }
  static void destroySuite( SaveNXcanSASTest *suite ) { delete suite; }


  void test_Something()
  {
    TS_FAIL( "You forgot to write a test!");
  }


};


#endif /* MANTID_DATAHANDLING_SAVENXCANSASTEST_H_ */