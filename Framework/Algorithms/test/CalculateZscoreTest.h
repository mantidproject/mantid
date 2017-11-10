#ifndef MANTID_ALGORITHMS_CALCULATEZSCORETEST_H_
#define MANTID_ALGORITHMS_CALCULATEZSCORETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAlgorithms/CalculateZscore.h"
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

    auto &Zscore = outWS->y(0);
    TS_ASSERT_DELTA(Zscore[4], 1.6397, 0.0001);
    TS_ASSERT_DELTA(Zscore[6], 0.3223, 0.0001);

    auto &histX = outWS->x(0);
    TS_ASSERT_DELTA(histX[0], 0.0, 0.000001);
    TS_ASSERT_DELTA(histX[5], 5.0, 0.000001);
    TS_ASSERT_DELTA(histX[10], 10.0, 0.000001);

    return;
  }

  /** Generate a workspace for test
    */
  MatrixWorkspace_sptr generateTestWorkspace() {
    vector<double> data{12, 13, 9,  18, 7,  9,  14, 16, 10, 12,
                        7,  13, 14, 19, 10, 16, 12, 16, 19, 11};

    MatrixWorkspace_sptr ws = boost::dynamic_pointer_cast<MatrixWorkspace>(
        WorkspaceFactory::Instance().create("Workspace2D", 1, data.size(),
                                            data.size()));

    auto &histX = ws->mutableX(0);
    auto &histY = ws->mutableY(0);
    auto &histE = ws->mutableE(0);

    for (size_t i = 0; i < data.size(); ++i) {
      histX[i] = static_cast<double>(i);
      histY[i] = data[i];
      histE[i] = sqrt(data[i]);
    }

    return ws;
  }
};

#endif /* MANTID_ALGORITHMS_CALCULATEZSCORETEST_H_ */
