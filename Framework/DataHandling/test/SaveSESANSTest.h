#ifndef MANTID_DATAHANDLING_SAVESESANSTEST_H_
#define MANTID_DATAHANDLING_SAVESESANSTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/SaveSESANS.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using Mantid::DataHandling::SaveSESANS;

class SaveSESANSTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SaveSESANSTest *createSuite() { return new SaveSESANSTest(); }
  static void destroySuite( SaveSESANSTest *suite ) { delete suite; }


  void test_rejectTooManySpectra() {
	  Mantid::API::MatrixWorkspace_sptr ws = WorkspaceCreationHelper::create2DWorkspace(10, 10);

	  Mantid::DataHandling::SaveSESANS testAlg;
	  testAlg.initialize();
	  testAlg.setChild(true);
	  testAlg.setRethrows(true);
	  testAlg.setProperty("InputWorkspace", ws);
	  testAlg.setProperty("Filename", "test.ses");
	  TS_ASSERT_THROWS(testAlg.execute(), std::runtime_error);
  }
};


#endif /* MANTID_DATAHANDLING_SAVESESANSTEST_H_ */