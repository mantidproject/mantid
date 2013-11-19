#ifndef MANTID_DATAHANDLING_LOADILLASCIITEST_H_
#define MANTID_DATAHANDLING_LOADILLASCIITEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/LoadILLAscii.h"

using Mantid::DataHandling::LoadILLAscii;
using namespace Mantid::API;

class LoadILLAsciiTest: public CxxTest::TestSuite {
public:
	// This pair of boilerplate methods prevent the suite being created statically
	// This means the constructor isn't called when running other tests
	static LoadILLAsciiTest *createSuite() {
		return new LoadILLAsciiTest();
	}
	static void destroySuite(LoadILLAsciiTest *suite) {
		delete suite;
	}

	LoadILLAsciiTest() :
			m_testFile("ILLD2B_123944") {
	}

	void test_Init() {
		LoadILLAscii alg;
		TS_ASSERT_THROWS_NOTHING(alg.initialize())
		TS_ASSERT(alg.isInitialized())
	}

	void test_exec() {
		// Name of the output workspace.
		std::string outWSName("LoadILLAsciiTest_OutputWS");

		LoadILLAscii alg;
		TS_ASSERT_THROWS_NOTHING(alg.initialize())
		TS_ASSERT(alg.isInitialized())
		TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("Filename", m_testFile));
		TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputWorkspacePrefix", "value"));
		TS_ASSERT_THROWS_NOTHING( alg.execute(); );
		TS_ASSERT(alg.isExecuted( ));

//    // Retrieve the workspace from data service. TODO: Change to your desired type
//    Workspace_sptr ws;
//    TS_ASSERT_THROWS_NOTHING( ws = AnalysisDataService::Instance().retrieveWS<Workspace>(outWSName) );
//    TS_ASSERT(ws);
//    if (!ws) return;
//
//    // TODO: Check the results
//
//    // Remove workspace from the data service.
//    AnalysisDataService::Instance().remove(outWSName);
	}

private:
	std::string m_testFile;

};

#endif /* MANTID_DATAHANDLING_LOADILLASCIITEST_H_ */
