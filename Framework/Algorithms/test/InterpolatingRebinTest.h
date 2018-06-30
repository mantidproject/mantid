#ifndef INTERPOLATINGREBINTEST_H_
#define INTERPOLATINGREBINTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAlgorithms/InterpolatingRebin.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidHistogramData/LinearGenerator.h"
#include <limits>

using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;
using namespace Mantid::API;
using namespace Mantid::Algorithms;
using Mantid::HistogramData::LinearGenerator;

class InterpolatingRebinTest : public CxxTest::TestSuite {
public:
  void testWorkspace_dist() {
    Workspace2D_sptr test_in1D = Create1DData();
    test_in1D->setDistribution(true);
    AnalysisDataService::Instance().add("InterpolatingRebinTest_indist",
                                        test_in1D);

    InterpolatingRebin rebin;
    rebin.initialize();
    rebin.setPropertyValue("InputWorkspace", "InterpolatingRebinTest_indist");
    rebin.setPropertyValue("OutputWorkspace", "InterpolatingRebinTest_outdist");
    // Check it fails if property not set
    TS_ASSERT_THROWS(rebin.execute(), std::runtime_error)
    TS_ASSERT(!rebin.isExecuted())

    // the last bin would are too high to calculate, check it aborts
    rebin.setPropertyValue("Params", "1,1,50");
    TS_ASSERT(!rebin.isExecuted())

    // some of the new bins would are too low to calculate, check it aborts
    rebin.setPropertyValue("Params", "0.85,0.001,15");
    TS_ASSERT(!rebin.isExecuted())

    // set the new bins to be less than half the size of the old, one in every 2
    // old bins and one in every 5 old will coinside
    rebin.setPropertyValue("Params", "2.225,0.2,15");
    TS_ASSERT_THROWS_NOTHING(rebin.execute())
    TS_ASSERT(rebin.isExecuted())

    // get the output workspace and test it
    MatrixWorkspace_sptr rebindata =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            "InterpolatingRebinTest_outdist");
    TS_ASSERT_EQUALS(rebindata->getNumberHistograms(), 1);
    TS_ASSERT(rebindata)

    const auto &outX = rebindata->x(0);
    const auto &outY = rebindata->y(0);
    const auto &outE = rebindata->e(0);
    TS_ASSERT_EQUALS(outX.size(), ceil((15 - 2.225) / 0.2) + 1);
    TS_ASSERT_EQUALS(outY.size(), ceil((15 - 2.225) / 0.2));
    TS_ASSERT_EQUALS(outE.size(), ceil((15 - 2.225) / 0.2));

    // this intepolated data was found by running the debugger on this test
    TS_ASSERT_DELTA(outX[0], 2.225, 0.00001);
    TS_ASSERT_DELTA(outY[0], 3.9, 0.0001);
    TS_ASSERT_DELTA(outE[0], 0.4875, 0.0001);

    // another output point between input points
    TS_ASSERT_DELTA(outX[7], 3.625, 0.00001);
    TS_ASSERT_DELTA(outY[7], 6.7, 0.0001);
    TS_ASSERT_DELTA(outE[7], 0.8375, 0.0001);

    // it is set up with the 49th output point being the same as the 15th input
    TS_ASSERT_DELTA(outX[49], 12.025, 0.00001);
    TS_ASSERT_DELTA(outY[49], (15 * 1.5) + 1, 0.0001);
    TS_ASSERT_DELTA(outE[49], (15 * 1.5 + 1) / 8.0, 0.0001);

    // the data is monotomically increasing and so the next out point should
    // have higher values than the previous but no as high as the next input
    // data point
    TS_ASSERT(outY[50] > (15 * 1.5) + 1);
    TS_ASSERT(outY[50] < (16 * 1.5) + 1);
    // errors -same thing
    TS_ASSERT(outE[50] > (15 * 1.5 + 1) / 8.0);
    TS_ASSERT(outE[50] < (16 * 1.5 + 1) / 8.0);

    // check the last point
    TS_ASSERT_DELTA(outX[64], 15, 0.00001);
    TS_ASSERT_DELTA(outY[63], 29.0749, 0.0001);
    TS_ASSERT_DELTA(outE[63], 3.6343, 0.0001);

    TS_ASSERT(rebindata->isDistribution());
    AnalysisDataService::Instance().remove("InterpolatingRebinTest_indist");
    AnalysisDataService::Instance().remove("InterpolatingRebinTest_outdist");
  }

  void testWorkspace_nondist() {

    Workspace2D_sptr test_in1D = Create1DData();
    test_in1D->setDistribution(false);
    AnalysisDataService::Instance().add("InterpolatingRebinTest_in_nondist",
                                        test_in1D);

    InterpolatingRebin rebin;
    rebin.initialize();
    rebin.setPropertyValue("InputWorkspace",
                           "InterpolatingRebinTest_in_nondist");
    rebin.setPropertyValue("OutputWorkspace",
                           "InterpolatingRebinTest_out_nondist");

    // set the new bins to be less than half the size of the old, one in every 2
    // old bins and one in every 5 old will coinside
    rebin.setPropertyValue("Params", "2.225,0.2,15");
    TS_ASSERT_THROWS_NOTHING(rebin.execute())
    TS_ASSERT(rebin.isExecuted())

    // get the output workspace and test it
    MatrixWorkspace_sptr rebindata =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            "InterpolatingRebinTest_out_nondist");
    TS_ASSERT(rebindata)
    const auto &outX = rebindata->x(0);
    const auto &outY = rebindata->y(0);
    const auto &outE = rebindata->e(0);

    // this intepolated data was found by running the debugger on this test
    TS_ASSERT_DELTA(outX[0], 2.225, 0.00001);
    TS_ASSERT_DELTA(outY[0], 1.0400, 0.0001);
    TS_ASSERT_DELTA(outE[0], 0.1300, 0.0001);

    ////another output point between input points
    TS_ASSERT_DELTA(outX[7], 3.625, 0.00001);
    TS_ASSERT_DELTA(outY[7], 1.7866, 0.0001);
    TS_ASSERT_DELTA(outE[7], 0.2233, 0.0001);

    // it is set up with the 49th output point being the same as the 15th input
    TS_ASSERT_DELTA(outX[49], 12.025, 0.00001);
    double origY = (15 * 1.5) + 1;
    double nondistY = origY / 0.75;
    double interpY = nondistY * 0.2;
    TS_ASSERT_DELTA(outY[49], interpY, 0.0001);
    TS_ASSERT_DELTA(outE[49], interpY / 8.0, 0.0001);

    // the data is monotomically increasing and so the next out point should
    // have higher values than the previous
    TS_ASSERT(outY[50] > interpY);
    // same with the error
    TS_ASSERT(outE[50] < (16 * 1.5 + 1) / 8.0);

    // check the last point
    TS_ASSERT_DELTA(outX[64], 15, 0.00001);
    TS_ASSERT_DELTA(outY[63], 6.7841, 0.0001);
    TS_ASSERT_DELTA(outE[63], 0.8480, 0.0001);

    // the distribution state of the output workspace should match that of the
    // input
    TS_ASSERT(!rebindata->isDistribution());
    AnalysisDataService::Instance().remove("InterpolatingRebinTest_in_nondist");
    AnalysisDataService::Instance().remove(
        "InterpolatingRebinTest_out_nondist");
  }

  void testWorkspace_close() {
    Workspace2D_sptr test_in1D = Create1DData();
    test_in1D->setDistribution(true);
    AnalysisDataService::Instance().add("InterpolatingRebinTest_inclose",
                                        test_in1D);

    InterpolatingRebin rebin;
    rebin.initialize();
    rebin.setPropertyValue("InputWorkspace", "InterpolatingRebinTest_inclose");
    rebin.setPropertyValue("OutputWorkspace",
                           "InterpolatingRebinTest_outclose");

    // the extreme values are just passed the ends of the data but the algorithm
    // assume they are on the boundary
    rebin.setPropertyValue("Params", "0.49999999,0.75,38.0000001");

    TS_ASSERT_THROWS_NOTHING(rebin.execute())
    TS_ASSERT(rebin.isExecuted())

    // get the output workspace and test it
    MatrixWorkspace_sptr rebindata =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            "InterpolatingRebinTest_outclose");
    TS_ASSERT(rebindata)

    const auto &outX = rebindata->x(0);
    const auto &outY = rebindata->y(0);
    const auto &outE = rebindata->e(0);
    // the output workspace should be the same as the input
    TS_ASSERT_EQUALS(outX.size(), test_in1D->x(0).size())
    TS_ASSERT_EQUALS(outY.size(), test_in1D->y(0).size())

    TS_ASSERT_DELTA(outX[0], 0.49999999, 0.00001)
    TS_ASSERT_DELTA(outY[0], 1, 0.00001)
    TS_ASSERT_DELTA(outE[0], 1.0 / 8.0, 0.0001);

    TS_ASSERT_DELTA(outX.back(), 38.00000, 0.00001);
    TS_ASSERT_DELTA(outY.back(), 74.5, 0.0001);

    TS_ASSERT(rebindata->isDistribution());
    AnalysisDataService::Instance().remove("InterpolatingRebinTest_inclose");
    AnalysisDataService::Instance().remove("InterpolatingRebinTest_outclose");
  }

  void testNullDataHandling() {

    Workspace2D_sptr test_in1D = badData();
    test_in1D->setDistribution(true);
    AnalysisDataService::Instance().add("InterpolatingRebinTest_in_nulldata",
                                        test_in1D);

    InterpolatingRebin rebin;
    rebin.initialize();
    rebin.setPropertyValue("InputWorkspace",
                           "InterpolatingRebinTest_in_nulldata");
    rebin.setPropertyValue("OutputWorkspace",
                           "InterpolatingRebinTest_out_nulldata");

    // set the new bins to be less than half the size of the old, one in every 2
    // old bins and one in every 5 old will coinside
    rebin.setPropertyValue("Params", "2,0.2,11");
    TS_ASSERT_THROWS_NOTHING(rebin.execute())
    TS_ASSERT(rebin.isExecuted())

    // get the output workspace and test it
    MatrixWorkspace_sptr rebindata =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            "InterpolatingRebinTest_out_nulldata");
    TS_ASSERT_EQUALS(rebindata->getNumberHistograms(), 2);

    const auto &outX = rebindata->x(0);
    const auto &outY = rebindata->y(0);
    const auto &outE = rebindata->e(0);
    TS_ASSERT_EQUALS(outX.size(), ceil((11 - 2.0) / 0.2) + 1);
    TS_ASSERT_EQUALS(outY.size(), ceil((11 - 2.0) / 0.2));
    TS_ASSERT_EQUALS(outE.size(), ceil((11 - 2.0) / 0.2));

    // the first spectrum should be only zeros test the first spectrum
    TS_ASSERT_DELTA(outX[0], 2, 0.00001);
    TS_ASSERT_DELTA(outY[0], 0, 0.0001);
    TS_ASSERT_DELTA(outE[0], 0, 0.0001);

    // test a random location
    TS_ASSERT_DELTA(outX[2], 2.4, 0.00001);
    TS_ASSERT_DELTA(outY[2], 0, 0.0001);
    TS_ASSERT_DELTA(outE[2], 0, 0.0001);

    // check the last point
    TS_ASSERT_DELTA(outX[45], 11, 0.00001);
    TS_ASSERT_DELTA(outY[44], 0, 0.0001);
    TS_ASSERT_DELTA(outE[44], 0, 0.0001);

    // the second spectrum is NAN
    const auto &outX1 = rebindata->x(1);
    const auto &outY1 = rebindata->y(1);
    const auto &outE1 = rebindata->e(1);
    // test a random one
    TS_ASSERT_DELTA(outX1[7], 3.4, 0.00001);
    // check for numeric_limits<double>::quiet_NaN()
    TS_ASSERT(outY1[7] != outY1[7]);
    TS_ASSERT_DELTA(outE1[7], 2, 0.00001);

    AnalysisDataService::Instance().remove(
        "InterpolatingRebinTest_in_nulldata");
    AnalysisDataService::Instance().remove(
        "InterpolatingRebinTest_out_nulldata");
  }

private:
  Workspace2D_sptr Create1DData() {
    static const int nBins = 50;
    Workspace2D_sptr retVal(new Workspace2D);
    retVal->initialize(1, nBins + 1, nBins);

    double j = 1.0;
    int i = 0;

    auto &x = retVal->mutableX(0);
    auto &y = retVal->mutableY(0);
    auto &e = retVal->mutableE(0);
    const auto &yConst = retVal->y(0);
    for (; i < nBins; i++, j += 1.5) {
      x[i] = j * 0.5;
      y[i] = j;
      e[i] = yConst[i] / 8;
    }
    x[i] = j * 0.5;

    return retVal;
  }

  Workspace2D_sptr badData() {
    static const int nBins = 24;
    Workspace2D_sptr retVal(new Workspace2D);
    retVal->initialize(2, nBins + 1, nBins);

    // the first histogram has all zeros
    retVal->setBinEdges(0, nBins + 1, LinearGenerator(0.0, 1.0));
    retVal->mutableY(0).assign(retVal->mutableY(0).size(), 0.0);
    retVal->mutableE(0).assign(retVal->mutableE(0).size(), 0.0);

    // the second has NAN values
    retVal->setBinEdges(1, nBins + 1, LinearGenerator(0.0, 1.0));
    retVal->mutableY(1).assign(retVal->mutableY(1).size(),
                               std::numeric_limits<double>::quiet_NaN());
    retVal->mutableE(1).assign(retVal->mutableE(1).size(), 2.0);

    return retVal;
  }
};
#endif /* INTERPOLATINGREBINTEST */
