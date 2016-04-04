#ifndef MANTID_DATAHANDLING_H5UTILTEST_H_
#define MANTID_DATAHANDLING_H5UTILTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/H5Util.h"

//using Mantid::DataHandling::H5Util;

class H5UtilTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static H5UtilTest *createSuite() { return new H5UtilTest(); }
  static void destroySuite( H5UtilTest *suite ) { delete suite; }


  void test_Something()
  {
    TS_FAIL( "You forgot to write a test!");
  }


};


#endif /* MANTID_DATAHANDLING_H5UTILTEST_H_ */
