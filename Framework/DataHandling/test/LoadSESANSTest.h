#ifndef MANTID_DATAHANDLING_LOADSESANSTEST_H_
#define MANTID_DATAHANDLING_LOADSESANSTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/LoadSESANS.h"

using Mantid::DataHandling::LoadSESANS;

class LoadSESANSTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static LoadSESANSTest *createSuite() { return new LoadSESANSTest(); }
  static void destroySuite( LoadSESANSTest *suite ) { delete suite; }

  void test_readHeaders() {
	  Mantid::DataHandling::LoadSESANS testAlg;
	  testAlg.initialize();
	  testAlg.setChild(true);
	  testAlg.setRethrows(true);
	  testAlg.setProperty("Filename", "test.ses");
	  testAlg.setProperty("OutputWorkspace", "ws");
	  TS_ASSERT_THROWS_NOTHING(testAlg.execute());
  }


};


#endif /* MANTID_DATAHANDLING_LOADSESANSTEST_H_ */