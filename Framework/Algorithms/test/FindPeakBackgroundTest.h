#ifndef MANTID_ALGORITHMS_FindPeakBackground_H_
#define MANTID_ALGORITHMS_FindPeakBackgroundTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/FindPeakBackground.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"

#include <numeric>
#include <cmath>

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;
using HistogramData::Points;
using HistogramData::Counts;

using namespace std;

using Mantid::Algorithms::FindPeakBackground;

class FindPeakBackgroundTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static FindPeakBackgroundTest *createSuite() {
    return new FindPeakBackgroundTest();
  }
  static void destroySuite(FindPeakBackgroundTest *suite) { delete suite; }

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

    Mantid::API::ITableWorkspace_sptr peaklist =
        boost::dynamic_pointer_cast<Mantid::API::ITableWorkspace>(
            Mantid::API::AnalysisDataService::Instance().retrieve("Signal"));

    TS_ASSERT(peaklist);
    TS_ASSERT_EQUALS(peaklist->rowCount(), 1);
    TS_ASSERT_DELTA(peaklist->Int(0, 1), 4, 0.01);
    TS_ASSERT_DELTA(peaklist->Int(0, 2), 19, 0.01);
    TS_ASSERT_DELTA(peaklist->Double(0, 3), 1.2, 0.01);
    TS_ASSERT_DELTA(peaklist->Double(0, 4), 0.04, 0.01);
    TS_ASSERT_DELTA(peaklist->Double(0, 5), 0.0, 0.01);

    // Clean
    AnalysisDataService::Instance().remove("Signal");

    return;
  }

  /** Generate a workspace for test
   */
  MatrixWorkspace_sptr generateTestWorkspace() {

    const size_t size = 20;

    MatrixWorkspace_sptr ws = boost::dynamic_pointer_cast<MatrixWorkspace>(
        WorkspaceFactory::Instance().create("Workspace2D", 1, size, size));

    MantidVec xdata(size);
    std::iota(xdata.begin(), xdata.end(), 0);

    ws->setHistogram(0, Points(xdata),
                     Counts{1, 2, 1, 1, 9, 11, 13, 20, 24, 32, 28, 48, 42, 77,
                            67, 33, 27, 20, 9, 2});
    return ws;
  }

  //--------------------------------------------------------------------------------------------
  /** Test on a spectrum without peak
    */
  void test_FindBackgroundOnFlat() {
    // Add workspace
    MatrixWorkspace_sptr testws = generate2SpectraTestWorkspace();
    AnalysisDataService::Instance().addOrReplace("Test2Workspace", testws);

    // Set up algorithm
    Algorithms::FindPeakBackground alg;

    alg.initialize();
    TS_ASSERT(alg.isInitialized());

    alg.setProperty("InputWorkspace", "Test2Workspace");
    alg.setProperty("OutputWorkspace", "Signal3");
    alg.setProperty("WorkspaceIndex", 0);

    // Execute
    alg.execute();
    TS_ASSERT(alg.isExecuted());

    // Check result
    ITableWorkspace_sptr outws = boost::dynamic_pointer_cast<ITableWorkspace>(
        AnalysisDataService::Instance().retrieve("Signal3"));
    TS_ASSERT(outws);
    if (!outws)
      return;

    TS_ASSERT_EQUALS(outws->rowCount(), 1);
    if (outws->rowCount() < 1)
      return;

    int ipeakmin = outws->Int(0, 1);
    int ipeakmax = outws->Int(0, 2);
    TS_ASSERT(ipeakmin >= ipeakmax);

    // Clean
    AnalysisDataService::Instance().remove("Signal3");
    AnalysisDataService::Instance().remove("Test2Workspace");

    return;
  }

  //--------------------------------------------------------------------------------------------
  /** Test on a spectrum without peak
    */
  void test_FindBackgroundOnSpec1() {
    // Add workspace
    MatrixWorkspace_sptr testws = generate2SpectraTestWorkspace();
    AnalysisDataService::Instance().addOrReplace("Test2Workspace", testws);

    // Set up algorithm
    Algorithms::FindPeakBackground alg;

    alg.initialize();
    TS_ASSERT(alg.isInitialized());

    alg.setProperty("InputWorkspace", "Test2Workspace");
    alg.setProperty("OutputWorkspace", "Signal2");
    alg.setProperty("WorkspaceIndex", 1);

    // Execute
    alg.execute();
    TS_ASSERT(alg.isExecuted());

    // Check result
    ITableWorkspace_sptr outws = boost::dynamic_pointer_cast<ITableWorkspace>(
        AnalysisDataService::Instance().retrieve("Signal2"));
    TS_ASSERT(outws);
    if (!outws)
      return;

    TS_ASSERT_EQUALS(outws->rowCount(), 1);
    if (outws->rowCount() < 1)
      return;

    TS_ASSERT_EQUALS(outws->rowCount(), 1);
    TS_ASSERT_DELTA(outws->Int(0, 1), 4, 0.01);
    TS_ASSERT_DELTA(outws->Int(0, 2), 19, 0.01);
    TS_ASSERT_DELTA(outws->Double(0, 3), 1.2, 0.01);
    TS_ASSERT_DELTA(outws->Double(0, 4), 0.04, 0.01);
    TS_ASSERT_DELTA(outws->Double(0, 5), 0.0, 0.01);

    // Clean
    AnalysisDataService::Instance().remove("Signal2");
    AnalysisDataService::Instance().remove("Test2Workspace");

    return;
  }

  //--------------------------------------------------------------------------------------------
  /** Generate a workspace with 2 spectra for test
   */
  MatrixWorkspace_sptr generate2SpectraTestWorkspace() {

    const size_t size = 20;

    MatrixWorkspace_sptr ws = boost::dynamic_pointer_cast<MatrixWorkspace>(
        WorkspaceFactory::Instance().create("Workspace2D", 2, size, size));

    // Workspace index = 0
    MantidVec xdata(size);
    std::iota(xdata.begin(), xdata.end(), 0);
    ws->mutableX(0) = std::move(xdata);
    ws->mutableE(0).assign(size, 1.0);

    // Workspace index = 1
    ws->setHistogram(1, ws->points(0),
                     Counts{1, 2, 1, 1, 9, 11, 13, 20, 24, 32, 28, 48, 42, 77,
                            67, 33, 27, 20, 9, 2});

    return ws;
  }
};

#endif /* MANTID_ALGORITHMS_FindPeakBackgroundTEST_H_ */
