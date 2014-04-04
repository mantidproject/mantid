#ifndef MANTID_DATAHANDLING_LOADILLREFLECTOMETRYTEST_H_
#define MANTID_DATAHANDLING_LOADILLREFLECTOMETRYTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/LoadILLReflectometry.h"
#include "MantidAPI/AnalysisDataService.h"

using namespace Mantid::API;
using Mantid::DataHandling::LoadILLReflectometry;

class LoadILLReflectometryTest: public CxxTest::TestSuite {
public:
	// This pair of boilerplate methods prevent the suite being created statically
	// This means the constructor isn't called when running other tests
	static LoadILLReflectometryTest *createSuite() {
		return new LoadILLReflectometryTest();
	}
	static void destroySuite(LoadILLReflectometryTest *suite) {
		delete suite;
	}

	LoadILLReflectometryTest() :
			m_dataFile("ILLD17_111686.nxs") {
	}

	void test_Init() {
		LoadILLReflectometry loader;
		TS_ASSERT_THROWS_NOTHING(loader.initialize())
		TS_ASSERT(loader.isInitialized())
	}

	void testName() {
		LoadILLReflectometry loader;
		TS_ASSERT_EQUALS(loader.name(), "LoadILLReflectometry");
	}

	void test_exec() {
		// Name of the output workspace.
		std::string outWSName("LoadILLReflectometryTest_OutputWS");

		LoadILLReflectometry loader;
		TS_ASSERT_THROWS_NOTHING(loader.initialize())
		TS_ASSERT(loader.isInitialized())
		TS_ASSERT_THROWS_NOTHING(
				loader.setPropertyValue("Filename", m_dataFile));
		TS_ASSERT_THROWS_NOTHING(
				loader.setPropertyValue("OutputWorkspace", outWSName));
		TS_ASSERT_THROWS_NOTHING(loader.execute()
		; );
		TS_ASSERT(loader.isExecuted());

		MatrixWorkspace_sptr output =
				AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
						outWSName);
		TS_ASSERT(output);

		if (!output)
			return;

		// TODO: Check the results

		// Remove workspace from the data service.
		AnalysisDataService::Instance().clear();
	}


private:
	std::string m_dataFile;

};

#endif /* MANTID_DATAHANDLING_LOADILLREFLECTOMETRYTEST_H_ */
