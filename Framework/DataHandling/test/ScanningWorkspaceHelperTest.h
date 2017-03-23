#ifndef MANTID_DATAHANDLING_SCANNINGWORKSPACEHELPERTEST_H_
#define MANTID_DATAHANDLING_SCANNINGWORKSPACEHELPERTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/ScanningWorkspaceHelper.h"

using Mantid::DataHandling::ScanningWorkspaceHelper;

class ScanningWorkspaceHelperTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ScanningWorkspaceHelperTest *createSuite() { return new ScanningWorkspaceHelperTest(); }
  static void destroySuite( ScanningWorkspaceHelperTest *suite ) { delete suite; }


  void test_Something()
  {
    TS_FAIL( "You forgot to write a test!");
  }


};


#endif /* MANTID_DATAHANDLING_SCANNINGWORKSPACEHELPERTEST_H_ */