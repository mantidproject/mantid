#ifndef MANTID_ALGORITHMS_CONVERTEMPTYTOTOFTEST_H_
#define MANTID_ALGORITHMS_CONVERTEMPTYTOTOFTEST_H_

#include <cxxtest/TestSuite.h>
#include <cmath>

#include "MantidAlgorithms/ConvertEmptyToTof.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidDataObjects/Workspace2D.h"

using Mantid::Algorithms::ConvertEmptyToTof;
using namespace Mantid;
using namespace API;

class ConvertEmptyToTofTest: public CxxTest::TestSuite {
public:
	// This pair of boilerplate methods prevent the suite being created statically
	// This means the constructor isn't called when running other tests
	static ConvertEmptyToTofTest *createSuite() {
		return new ConvertEmptyToTofTest();
	}
	static void destroySuite(ConvertEmptyToTofTest *suite) {
		delete suite;
	}

	void test_Init() {
		ConvertEmptyToTof alg;
		TS_ASSERT_THROWS_NOTHING(alg.initialize())
		TS_ASSERT(alg.isInitialized())
	}

	void test_exec() {
		// Name of the output workspace.
		std::string outWSName("ConvertEmptyToTofTest_OutputWS");
		std::string inWSName("ConvertEmptyToTofTest_InputWS");

		DataObjects::Workspace2D_sptr testWS = createTestWorkspace();
		WorkspaceCreationHelper::storeWS(inWSName, testWS);

		ConvertEmptyToTof alg;
		TS_ASSERT_THROWS_NOTHING(alg.initialize())
		TS_ASSERT(alg.isInitialized())
		TS_ASSERT_THROWS_NOTHING(
				alg.setPropertyValue("InputWorkspace", inWSName));
		TS_ASSERT_THROWS_NOTHING(
				alg.setPropertyValue("OutputWorkspace", outWSName));
		TS_ASSERT_THROWS_NOTHING(
				alg.setPropertyValue("ListOfSpectraIndices", "5"));
		TS_ASSERT_THROWS_NOTHING(
				alg.setPropertyValue("ListOfChannelIndices", "40-60"));
		TS_ASSERT_THROWS_NOTHING(alg.execute()
		; );
		TS_ASSERT(alg.isExecuted());

		// Retrieve the workspace from data service. TODO: Change to your desired type
		Workspace_sptr ws;
		TS_ASSERT_THROWS_NOTHING(
				ws = AnalysisDataService::Instance().retrieveWS<Workspace>(
						outWSName));
		TS_ASSERT(ws);
		if (!ws)
			return;

		// TODO: Check the results

		// Remove workspace from the data service.
		AnalysisDataService::Instance().remove(outWSName);
	}

private:
	DataObjects::Workspace2D_sptr createTestWorkspace() {

		// create test ws
		const size_t nHist = 10; //testWS->getNumberHistograms();
		const size_t nBins = 101; //testWS->blocksize();
		// int nHist, int nBins, bool includeMonitors = false,
        // bool startYNegative = false, bool isHistogram = true, const std::string& instrumentName = std::string("testInst")
		DataObjects::Workspace2D_sptr testWS =
				WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(nHist, nBins, false,
						false, true,  std::string("testInstEmpty"));
		testWS->getAxis(0)->setUnit("Empty");
		API::Run & run = testWS->mutableRun();
		run.addProperty<double>("wavelength",5.0,true); //overwrite
		run.addProperty<double>("channel_width",30.0,true); //overwrite


		for (size_t i = 0; i < nHist; ++i) {
			for (size_t j = 0; j < nBins - 1; ++j) {
				// gaussian peak centred at 50,and h=10
				testWS->dataY(i)[j] = 10 * exp(	-pow((static_cast<double>(j) - 50), 2)
										/ (2 * pow(1.5, 2)));
			}
		}
		return testWS;
	}

};

#endif /* MANTID_ALGORITHMS_CONVERTEMPTYTOTOFTEST_H_ */
