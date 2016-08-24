#ifndef MANTID_DATAHANDLING_LOADBANKFROMDISKTASKTEST_H_
#define MANTID_DATAHANDLING_LOADBANKFROMDISKTASKTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/LoadBankFromDiskTask.h"

using Mantid::DataHandling::LoadBankFromDiskTask;

class LoadBankFromDiskTaskTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static LoadBankFromDiskTaskTest *createSuite() {
    return new LoadBankFromDiskTaskTest();
  }
  static void destroySuite(LoadBankFromDiskTaskTest *suite) { delete suite; }

  void test_Something() { TS_FAIL("You forgot to write a test!"); }
};

#endif /* MANTID_DATAHANDLING_LOADBANKFROMDISKTASKTEST_H_ */