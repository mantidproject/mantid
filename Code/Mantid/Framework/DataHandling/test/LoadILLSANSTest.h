#ifndef MANTID_DATAHANDLING_LOADILLSANSTEST_H_
#define MANTID_DATAHANDLING_LOADILLSANSTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/LoadILLSANS.h"

using Mantid::DataHandling::LoadILLSANS;
using namespace Mantid::API;

class LoadILLSANSTest : public CxxTest::TestSuite
{
public:
	// This pair of boilerplate methods prevent the suite being created statically
		// This means the constructor isn't called when running other tests
		static LoadILLSANSTest *createSuite() {
			return new LoadILLSANSTest();
		}
		static void destroySuite(LoadILLSANSTest *suite) {
			delete suite;
		}

		LoadILLSANSTest() :
				m_testFile("ILLD33_001030.nxs") {
		}
		void testName() {
			LoadILLSANS alg;
			TS_ASSERT_EQUALS(alg.name(), "LoadILLSANS");
		}

		void testVersion() {
			LoadILLSANS alg;
			TS_ASSERT_EQUALS(alg.version(), 1);
		}

		void test_Init() {
			LoadILLSANS alg;
			TS_ASSERT_THROWS_NOTHING(alg.initialize())
			TS_ASSERT(alg.isInitialized())
		}

		void test_exec() {
			LoadILLSANS loader;
			loader.initialize();
			loader.setPropertyValue("Filename", m_testFile);

			std::string outputSpace = "LoadILLSANSTest_out";
			loader.setPropertyValue("OutputWorkspace", outputSpace);
			TS_ASSERT_THROWS_NOTHING(loader.execute());

			//  test workspace, copied from LoadMuonNexusTest.h
			MatrixWorkspace_sptr output;

			(output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
					outputSpace));
			MatrixWorkspace_sptr output2D = boost::dynamic_pointer_cast<
					MatrixWorkspace>(output);

			TS_ASSERT_EQUALS(output2D->getNumberHistograms(), 65536);
			TS_ASSERT_EQUALS(output2D->blocksize(), 100);
			AnalysisDataService::Instance().clear();
		}


	private:
		std::string m_testFile;


};


#endif /* MANTID_DATAHANDLING_LOADILLSANSTEST_H_ */
