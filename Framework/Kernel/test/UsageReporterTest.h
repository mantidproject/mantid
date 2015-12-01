#ifndef MANTID_API_USAGESERVICETEST_H_
#define MANTID_API_USAGESERVICETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidKernel/UsageReporter.h"

using Mantid::Kernel::UsageReporter;

class UsageServiceTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static UsageServiceTest *createSuite() { return new UsageServiceTest(); }
  static void destroySuite( UsageServiceTest *suite ) { delete suite; }


  void test_Something()
  {
    TS_FAIL( "You forgot to write a test!");
  }


};


#endif /* MANTID_API_USAGESERVICETEST_H_ */