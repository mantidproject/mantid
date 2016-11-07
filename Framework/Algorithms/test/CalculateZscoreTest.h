#ifndef MANTID_ALGORITHMS_CALCULATEZSCORETEST_H_
#define MANTID_ALGORITHMS_CALCULATEZSCORETEST_H_

#include <cxxtest/TestSuite.h>

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

class CalculateZscoreTestPerformance : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CalculateZscoreTestPerformance *createSuite() {
    return new CalculateZscoreTestPerformance();
  }
  static void destroySuite(CalculateZscoreTestPerformance *suite) {
    delete suite;
  }

  CalculateZscoreTestPerformance() {
    inWS_small = create2DWorkspace(50, 50);
    inWS_medium = create2DWorkspace(200, 200);
    inWS_large = create2DWorkspace(500, 500);
  }

  void testPerformanceSmall() { runTest(inWS_small); }
  void testPerformanceMedium() { runTest(inWS_medium); }
  void testPerformanceLarge() { runTest(inWS_large); }

private:
  MatrixWorkspace_sptr inWS_small;
  MatrixWorkspace_sptr inWS_medium;
  MatrixWorkspace_sptr inWS_large;

  void runTest(MatrixWorkspace_sptr inWS) {
    // 2. Create
    Algorithms::CalculateZscore alg;

    alg.initialize();
    TS_ASSERT(alg.isInitialized());

    alg.setProperty("InputWorkspace", inWS);
    alg.setProperty("OutputWorkspace", "Zscores");
    // no workspace index so that it runs on _ALL_ histograms
    // alg.setProperty("WorkspaceIndex", 1);

    alg.execute();
    TS_ASSERT(alg.isExecuted());
  }

  MatrixWorkspace_sptr create2DWorkspace(const size_t histSize = 200,
                                         const size_t pointSize = 200) {
    vector<double> data{};
    data.reserve(pointSize);
    for (size_t i = 0; i < pointSize; ++i) {
      data.push_back(theGiftThatKeepsOnGiving<double>());
    }

    auto Xsize = data.size(), Ysize = data.size();

    MatrixWorkspace_sptr ws = boost::dynamic_pointer_cast<MatrixWorkspace>(
        WorkspaceFactory::Instance().create("Workspace2D", histSize, Xsize,
                                            Ysize));

    auto &histX = ws->mutableX(0);
    auto &histY = ws->mutableY(0);
    auto &histE = ws->mutableE(0);
    for (size_t i = 0; i < data.size(); ++i) {
      histX[i] = static_cast<double>(i);
      histY[i] = data[i];
      histE[i] = sqrt(data[i]);
    }

    auto sharedX = ws->sharedX(0);
    auto sharedY = ws->sharedY(0);
    auto sharedE = ws->sharedE(0);
    for (size_t i = 1; i < histSize; ++i) {
      ws->setSharedX(i, sharedX);
      ws->setSharedY(i, sharedY);
      ws->setSharedE(i, sharedE);
    }

    return ws;
  }

  /** Returns the same sequence of numbers forever
  */
  template <typename T> T theGiftThatKeepsOnGiving() {
    static std::vector<T> data = {12, 13, 9,  18, 7,  9,  14, 16, 10, 12,
                                  7,  13, 14, 19, 10, 16, 12, 16, 19, 11};
    static auto dataSt = data.begin();
    static auto dataEnd = data.end();

    // it keeps on repeating the same numbers forever by resetting after
    // reaching the last one
    return (dataSt != dataEnd) ? *dataSt++ : (dataSt = data.begin(), *dataSt);
  }
};

#endif /* MANTID_ALGORITHMS_CALCULATEZSCORETEST_H_ */
