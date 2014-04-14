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

		LoadILLAscii loader;
		loader.initialize();
		loader.setPropertyValue("Filename", m_testFile);
		std::string outputSpaceName = "LoadILLTest_out";
		loader.setPropertyValue("OutputWorkspace", outputSpaceName);
		TS_ASSERT_THROWS_NOTHING( loader.execute());

		// Retrieve the workspace from data service.
		IMDEventWorkspace_sptr ws;
		TS_ASSERT_THROWS_NOTHING(
				ws = AnalysisDataService::Instance().retrieveWS<IMDEventWorkspace>(outputSpaceName));
		TS_ASSERT(ws);

		//TS_ASSERT_EQUALS( ws->getNEvents(), 409600);
		// Trimmed file to be submitted to github
		TS_ASSERT_EQUALS( ws->getNEvents(), 114688);

		// Remove workspace from the data service.
		AnalysisDataService::Instance().remove(outputSpaceName);

	}

//	// DOES NOT WORK. Can't find input file
//	void test_LoadILLHelper() {
//
//		using Mantid::DataHandling::ILLParser;
//
//		// Parses ascii file and fills the data scructures
//		ILLParser illAsciiParser("../Test/AutoTestData/" + m_testFile);
//		illAsciiParser.parse();
//
//		double wavelength = illAsciiParser.getValueFromHeader<double>("wavelength");
//		TS_ASSERT_EQUALS(wavelength,25)
//
//		// get local references to the parsed file
//		const std::vector<std::vector<int> > &spectraList = illAsciiParser.getSpectraList();
//		const std::vector<std::map<std::string, std::string> > &spectraHeaderList = illAsciiParser.getSpectraHeaderList();
//
//		TS_ASSERT_EQUALS(spectraList.size(),25)
//		TS_ASSERT_EQUALS(spectraHeaderList.size(),25)
//
//	}

private:
	std::string m_testFile;

};

#endif /* MANTID_DATAHANDLING_LOADILLASCIITEST_H_ */
