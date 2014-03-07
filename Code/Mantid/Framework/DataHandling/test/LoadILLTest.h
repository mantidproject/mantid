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
			m_dataFile("ILLIN5_104007.nxs") {
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

	/*
	 * This test only loads the Sample Data
	 * The elastic peak is obtained on the fly from the sample data.
	 */
	void testExecJustSample() {
		LoadILL loader;
		loader.initialize();
		loader.setPropertyValue("Filename", m_dataFile);

		std::string outputSpace = "LoadILLTest_out";
		loader.setPropertyValue("OutputWorkspace", outputSpace);
		TS_ASSERT_THROWS_NOTHING( loader.execute());

		MatrixWorkspace_sptr output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outputSpace);
		MatrixWorkspace_sptr output2D = boost::dynamic_pointer_cast<MatrixWorkspace>(output);

		TS_ASSERT_EQUALS( output2D->getNumberHistograms(), 98304);

		AnalysisDataService::Instance().clear();
	}



private:
	std::string m_dataFile;

};

//------------------------------------------------------------------------------
// Performance test
//------------------------------------------------------------------------------

class LoadILLTestPerformance: public CxxTest::TestSuite {
public:
	LoadILLTestPerformance() :
		m_dataFile("ILLIN5_104007.nxs"){
	}

	void testDefaultLoad() {
		Mantid::DataHandling::LoadILL loader;
		loader.initialize();
		loader.setPropertyValue("Filename", m_dataFile);
		loader.setPropertyValue("OutputWorkspace", "ws");
		TS_ASSERT(loader.execute());
	}

private:
	std::string m_dataFile;

};

#endif /*LoadILLTEST_H_*/
