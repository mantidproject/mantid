#ifndef LOADILLTEST_H_
#define LOADILLTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidDataHandling/LoadILL.h"
#include "MantidAPI/AnalysisDataService.h"

using namespace Mantid::API;

class LoadILLTest: public CxxTest::TestSuite {
public:
	LoadILLTest() :
			testFile("ILLIN5_094460.nxs") {
	}
	void testName() {
		TS_ASSERT_EQUALS( loader.name(), "LoadILL");
	}

	void testVersion() {
		TS_ASSERT_EQUALS( loader.version(), 1);
	}

	void testInit() {
		TS_ASSERT_THROWS_NOTHING( loader.initialize());
		TS_ASSERT( loader.isInitialized());
	}

//	void testFileCheck() {
//		std::cerr << loader.fileCheck(testFile);
//	}

	void testExec() {
		loader.setPropertyValue("Filename", testFile);

		std::string outputSpace = "LoadILLTest_out";
		loader.setPropertyValue("OutputWorkspace", outputSpace);
		TS_ASSERT_THROWS_NOTHING( loader.execute());

		//  test workspace, copied from LoadMuonNexusTest.h
		MatrixWorkspace_sptr output;

		(output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
				outputSpace));
		MatrixWorkspace_sptr output2D = boost::dynamic_pointer_cast<
				MatrixWorkspace>(output);

		TS_ASSERT_EQUALS( output2D->getNumberHistograms(), 98304);

		AnalysisDataService::Instance().clear();
	}

private:
	Mantid::DataHandling::LoadILL loader;
	std::string testFile;
};

//------------------------------------------------------------------------------
// Performance test
//------------------------------------------------------------------------------

class LoadILLTestPerformance: public CxxTest::TestSuite {
public:
	void testDefaultLoad() {
		Mantid::DataHandling::LoadILL loader;
		loader.initialize();
		loader.setPropertyValue("Filename", "ILLIN5_094460.nxs");
		loader.setPropertyValue("OutputWorkspace", "ws");
		TS_ASSERT( loader.execute());
	}
};

#endif /*LoadILLTEST_H_*/
