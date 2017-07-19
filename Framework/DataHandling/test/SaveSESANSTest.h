#ifndef MANTID_DATAHANDLING_SAVESESANSTEST_H_
#define MANTID_DATAHANDLING_SAVESESANSTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/SaveSESANS.h"

using Mantid::DataHandling::SaveSESANS;

class SaveSESANSTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SaveSESANSTest *createSuite() { return new SaveSESANSTest(); }
  static void destroySuite( SaveSESANSTest *suite ) { delete suite; }


  void test_Something()
  {
    TS_FAIL( "You forgot to write a test!");
  }


};


#endif /* MANTID_DATAHANDLING_SAVESESANSTEST_H_ */