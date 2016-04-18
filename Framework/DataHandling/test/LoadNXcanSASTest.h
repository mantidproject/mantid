#ifndef MANTID_DATAHANDLING_LOADNXCANSASTEST_H_
#define MANTID_DATAHANDLING_LOADNXCANSASTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/LoadNXcanSAS.h"

using Mantid::DataHandling::LoadNXcanSAS;

class LoadNXcanSASTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static LoadNXcanSASTest *createSuite() { return new LoadNXcanSASTest(); }
  static void destroySuite(LoadNXcanSASTest *suite) { delete suite; }

  void test_Something() { TS_FAIL("You forgot to write a test!"); }
};

#endif /* MANTID_DATAHANDLING_LOADNXCANSASTEST_H_ */