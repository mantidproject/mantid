#ifndef MANTID_DATAHANDLING_LOADSWANSTEST_H_
#define MANTID_DATAHANDLING_LOADSWANSTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataHandling/LoadSwans.h"

using Mantid::DataHandling::LoadSwans;

class LoadSwansTest: public CxxTest::TestSuite {
public:
	// This pair of boilerplate methods prevent the suite being created statically
	// This means the constructor isn't called when running other tests
	static LoadSwansTest *createSuite() {
		return new LoadSwansTest();
	}
	static void destroySuite(LoadSwansTest *suite) {
		delete suite;
	}

	void test_Init() {
		LoadSwans alg;
		TS_ASSERT_THROWS_NOTHING(alg.initialize())
		TS_ASSERT(alg.isInitialized())
	}

	void test_exec() {
		// Data file
		std::string filename = "SWANS_RUN80814.dat";
		LoadSwans alg;

		alg.setChild(true);
		TS_ASSERT_THROWS_NOTHING(alg.initialize())
		TS_ASSERT(alg.isInitialized())

		TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", filename));
		TS_ASSERT_THROWS_NOTHING(
				alg.setPropertyValue("OutputWorkspace", "Output_ws_name"));

		TS_ASSERT_THROWS_NOTHING(alg.execute());
		TS_ASSERT(alg.isExecuted());
		Mantid::DataObjects::EventWorkspace_sptr outputWS = alg.getProperty(
				"OutputWorkspace");

		TS_ASSERT_EQUALS(outputWS->getNumberHistograms(), 16384);

	}

};

#endif /* MANTID_DATAHANDLING_LOADSWANSTEST_H_ */
