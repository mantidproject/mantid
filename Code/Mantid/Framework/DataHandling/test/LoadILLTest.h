#ifndef LOADILLTEST_H_
#define LOADILLTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidDataHandling/LoadILL.h"
#include "MantidAPI/AnalysisDataService.h"

using namespace Mantid::API;
using Mantid::DataHandling::LoadILL;

class LoadILLTest: public CxxTest::TestSuite 
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static LoadILLTest *createSuite() { return new LoadILLTest(); }
  static void destroySuite( LoadILLTest *suite ) { delete suite; }

	LoadILLTest() :
			m_testFile("ILLIN5_104007.nxs")
	{
	}

	void testName() {
		LoadILL loader;
		TS_ASSERT_EQUALS( loader.name(), "LoadILL");
	}

	void testVersion() {
		LoadILL loader;
		TS_ASSERT_EQUALS( loader.version(), 1);
	}

	void testInit() {
		LoadILL loader;
		TS_ASSERT_THROWS_NOTHING( loader.initialize());
		TS_ASSERT( loader.isInitialized());
	}

//	void testFileCheck() {
//		std::cerr << loader.fileCheck(testFile);
//	}

	void testExec() {
		LoadILL loader;
		loader.initialize();
		loader.setPropertyValue("Filename", m_testFile);

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

	std::string m_testFile;
};

//------------------------------------------------------------------------------
// Performance test
//------------------------------------------------------------------------------

class LoadILLTestPerformance: public CxxTest::TestSuite {
public:
	void testDefaultLoad() {
		Mantid::DataHandling::LoadILL loader;
		loader.initialize();
		loader.setPropertyValue("Filename", "ILLIN5_104007.nxs");
		loader.setPropertyValue("OutputWorkspace", "ws");
		TS_ASSERT( loader.execute());
	}
};

#endif /*LoadILLTEST_H_*/
