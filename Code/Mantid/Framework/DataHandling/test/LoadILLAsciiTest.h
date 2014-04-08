#ifndef MANTID_DATAHANDLING_LOADILLASCIITEST_H_
#define MANTID_DATAHANDLING_LOADILLASCIITEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/LoadILLAscii.h"
#include "MantidDataHandling/LoadILLAsciiHelper.h"

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

		LoadILLAscii alg;
		alg.setRethrows(true);
		TS_ASSERT_THROWS_NOTHING(alg.initialize())
		TS_ASSERT(alg.isInitialized())
		TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", m_testFile));
		TS_ASSERT_THROWS_NOTHING(
				alg.setPropertyValue("OutputWorkspace", "outputWSName"));
		TS_ASSERT_THROWS_NOTHING(alg.execute());
		TS_ASSERT(alg.isExecuted());

		// Retrieve the workspace from data service. TODO: Change to your desired type
		IMDEventWorkspace_sptr ws;
		TS_ASSERT_THROWS_NOTHING(
				ws = AnalysisDataService::Instance().retrieveWS<IMDEventWorkspace>("outputWSName"));
		TS_ASSERT(ws);

		TS_ASSERT_EQUALS( ws->getNEvents(), 409600);

		// Remove workspace from the data service.
		AnalysisDataService::Instance().remove("outputWSName");

	}

	void test_LoadILLHelper() {

		using Mantid::DataHandling::ILLParser;

		// Parses ascii file and fills the data scructures
		ILLParser illAsciiParser(m_testFile);
		illAsciiParser.parse();

		// get local references to the parsed file
		// const std::vector<std::vector<int> > &spectraList = illAsciiParser.getSpectraList();
		// const std::vector<std::map<std::string, std::string> > &spectraHeaderList = illAsciiParser.getSpectraHeaderList();

	}

private:
	std::string m_testFile;

};

#endif /* MANTID_DATAHANDLING_LOADILLASCIITEST_H_ */
