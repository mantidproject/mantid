#ifndef MANTID_ALGORITHMS_CALCULATEZSCORETEST_H_
#define MANTID_ALGORITHMS_CALCULATEZSCORETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/CalculateZscore.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"

#include <cmath>

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;

using namespace std;

using Mantid::Algorithms::CalculateZscore;

class CalculateZscoreTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CalculateZscoreTest *createSuite() {
    return new CalculateZscoreTest();
  }
  static void destroySuite(CalculateZscoreTest *suite) { delete suite; }

  void test_Calculation() {

    // 1. Generate input workspace
    MatrixWorkspace_sptr inWS = generateTestWorkspace();

    // 2. Create
    Algorithms::CalculateZscore alg;

    alg.initialize();
    TS_ASSERT(alg.isInitialized());

    alg.setProperty("InputWorkspace", inWS);
    alg.setProperty("OutputWorkspace", "Zscores");
    alg.setProperty("WorkspaceIndex", 0);

    alg.execute();
    TS_ASSERT(alg.isExecuted());

    Workspace2D_sptr outWS = boost::dynamic_pointer_cast<Workspace2D>(
        AnalysisDataService::Instance().retrieve("Zscores"));

    const MantidVec &Zscore = outWS->readY(0);
    TS_ASSERT_DELTA(Zscore[4], 1.6397, 0.0001);
    TS_ASSERT_DELTA(Zscore[6], 0.3223, 0.0001);

    const MantidVec &vecX = outWS->readX(0);
    TS_ASSERT_DELTA(vecX[0], 0.0, 0.000001);
    TS_ASSERT_DELTA(vecX[5], 5.0, 0.000001);
    TS_ASSERT_DELTA(vecX[10], 10.0, 0.000001);

    return;
  }

  /** Generate a workspace for test
    */
  MatrixWorkspace_sptr generateTestWorkspace() {
    vector<double> data;
    data.push_back(12);
    data.push_back(13);
    data.push_back(9);
    data.push_back(18);
    data.push_back(7);
    data.push_back(9);
    data.push_back(14);
    data.push_back(16);
    data.push_back(10);
    data.push_back(12);
    data.push_back(7);
    data.push_back(13);
    data.push_back(14);
    data.push_back(19);
    data.push_back(10);
    data.push_back(16);
    data.push_back(12);
    data.push_back(16);
    data.push_back(19);
    data.push_back(11);

    MatrixWorkspace_sptr ws = boost::dynamic_pointer_cast<MatrixWorkspace>(
        WorkspaceFactory::Instance().create("Workspace2D", 1, data.size(),
                                            data.size()));

    MantidVec &vecX = ws->dataX(0);
    MantidVec &vecY = ws->dataY(0);
    MantidVec &vecE = ws->dataE(0);

    for (size_t i = 0; i < data.size(); ++i) {
      vecX[i] = static_cast<double>(i);
      vecY[i] = data[i];
      vecE[i] = sqrt(data[i]);
    }

    return ws;
  }
};

#endif /* MANTID_ALGORITHMS_CALCULATEZSCORETEST_H_ */
