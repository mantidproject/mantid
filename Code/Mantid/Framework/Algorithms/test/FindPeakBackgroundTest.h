#ifndef MANTID_ALGORITHMS_FindPeakBackground_H_
#define MANTID_ALGORITHMS_FindPeakBackgroundTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/FindPeakBackground.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"

#include <cmath>

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;

using namespace std;

using Mantid::Algorithms::FindPeakBackground;

class FindPeakBackgroundTest: public CxxTest::TestSuite {
public:
	// This pair of boilerplate methods prevent the suite being created statically
	// This means the constructor isn't called when running other tests
	static FindPeakBackgroundTest *createSuite() {
		return new FindPeakBackgroundTest();
	}
	static void destroySuite(FindPeakBackgroundTest *suite) {
		delete suite;
	}

	void test_Calculation() {

		// 1. Generate input workspace
		MatrixWorkspace_sptr inWS = generateTestWorkspace();

		// 2. Create
		Algorithms::FindPeakBackground alg;

		alg.initialize();
		TS_ASSERT(alg.isInitialized());

		alg.setProperty("InputWorkspace", inWS);
		alg.setProperty("OutputWorkspace", "Signal");
		alg.setProperty("WorkspaceIndex", 0);

		alg.execute();
		TS_ASSERT(alg.isExecuted());

	    Mantid::API::ITableWorkspace_sptr peaklist = boost::dynamic_pointer_cast<Mantid::API::ITableWorkspace>
	                  (Mantid::API::AnalysisDataService::Instance().retrieve("Signal"));

	    TS_ASSERT( peaklist );
	    TS_ASSERT_EQUALS( peaklist->rowCount() , 1 );
	    TS_ASSERT_DELTA( peaklist->Int(0,1), 4, 0.01 );
	    TS_ASSERT_DELTA( peaklist->Int(0,2), 19, 0.01 );
	    TS_ASSERT_DELTA( peaklist->Double(0,3), 1.2, 0.01 );
	    TS_ASSERT_DELTA( peaklist->Double(0,4), 0.04, 0.01 );
	    TS_ASSERT_DELTA( peaklist->Double(0,5), 0.0, 0.01 );

		return;
	}

	/** Generate a workspace for test
	 */
	MatrixWorkspace_sptr generateTestWorkspace() {
		vector<double> data;
		data.push_back(1);
		data.push_back(2);
		data.push_back(1);
		data.push_back(1);
		data.push_back(9);
		data.push_back(11);
		data.push_back(13);
		data.push_back(20);
		data.push_back(24);
		data.push_back(32);
		data.push_back(28);
		data.push_back(48);
		data.push_back(42);
		data.push_back(77);
		data.push_back(67);
		data.push_back(33);
		data.push_back(27);
		data.push_back(20);
		data.push_back(9);
		data.push_back(2);

		MatrixWorkspace_sptr ws = boost::dynamic_pointer_cast<MatrixWorkspace>(
				WorkspaceFactory::Instance().create("Workspace2D", 1,
						data.size(), data.size()));

		MantidVec& vecX = ws->dataX(0);
		MantidVec& vecY = ws->dataY(0);
		MantidVec& vecE = ws->dataE(0);

		for (size_t i = 0; i < data.size(); ++i) {
			vecX[i] = static_cast<double> (i);
			vecY[i] = data[i];
			vecE[i] = sqrt(data[i]);
		}

		return ws;
	}

};

#endif /* MANTID_ALGORITHMS_FindPeakBackgroundTEST_H_ */
