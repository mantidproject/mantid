#ifndef MANTID_DATAHANDLING_LOADEVENTNEXUSINDEXSETUPTEST_H_
#define MANTID_DATAHANDLING_LOADEVENTNEXUSINDEXSETUPTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/LoadEventNexusIndexSetup.h"

using Mantid::DataHandling::LoadEventNexusIndexSetup;

class LoadEventNexusIndexSetupTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static LoadEventNexusIndexSetupTest *createSuite() { return new LoadEventNexusIndexSetupTest(); }
  static void destroySuite( LoadEventNexusIndexSetupTest *suite ) { delete suite; }


  void test_Something()
  {
    TS_FAIL( "You forgot to write a test!");
  }


};


#endif /* MANTID_DATAHANDLING_LOADEVENTNEXUSINDEXSETUPTEST_H_ */