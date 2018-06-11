#ifndef MANTID_ALGORITHMS_REBINRAGGEDTEST_H_
#define MANTID_ALGORITHMS_REBINRAGGEDTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAlgorithms/CreateWorkspace.h"
#include "MantidAlgorithms/ResampleX.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidHistogramData/LinearGenerator.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using Mantid::Algorithms::ResampleX;
using Mantid::HistogramData::BinEdges;
using Mantid::HistogramData::CountStandardDeviations;
using Mantid::HistogramData::Counts;
using Mantid::MantidVec;

class ResampleXTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ResampleXTest *createSuite() { return new ResampleXTest(); }
  static void destroySuite(ResampleXTest *suite) { delete suite; }

  void test_Init() {
    ResampleX alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
  }

  void test_linear_binning_histogram() {
    ResampleX alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());

    int numBins(3000);
    MantidVec xValues;
    double delta;

    // testing linear binning for histogram
    alg.setOptions(numBins, false, false);
    delta = alg.determineBinning(xValues, 0., 300.);
    //    std::cout << "**** " << numBins << " **** " << xValues.front() << ", "
    //    << delta << ", " << xValues.back() << " ****\n";
    //    std::cout << "000> " << xValues.size() << '\n';
    //    std::cout << "001> " << xValues[0] << ", " << xValues[1] << ", ..., "
    //    << xValues.back() << '\n';
    TS_ASSERT_EQUALS(numBins, xValues.size() - 1);
    TS_ASSERT_DELTA(0.1, delta, .001);
    TS_ASSERT_EQUALS(0.0, xValues[0]);
    TS_ASSERT_EQUALS(0.1, xValues[1]);
    TS_ASSERT_EQUALS(300., xValues[3000]);
  }

  void test_linear_binning_density() {
    ResampleX alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());

    int numBins(3000);
    MantidVec xValues;
    double delta;

    // testing linear binning for density
    alg.setOptions(numBins, false, true);
    delta = alg.determineBinning(xValues, 0.1, 300);
    //    std::cout << "**** " << numBins << " **** " << xValues.front() << ", "
    //    << delta << ", " << xValues.back() << " ****\n";
    //    std::cout << "000> " << xValues.size() << '\n';
    //    std::cout << "001> " << xValues[0] << ", " << xValues[1] << ", ..., "
    //    << xValues.back() << '\n';
    TS_ASSERT_EQUALS(numBins, xValues.size());
    TS_ASSERT_DELTA(0.1, delta, .001);
    TS_ASSERT_EQUALS(0.1, xValues[0]);
    TS_ASSERT_EQUALS(0.2, xValues[1]);
    TS_ASSERT_EQUALS(300., xValues[2999]);
  }

  void test_log_binning_histogram() {
    ResampleX alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());

    //
    int numBins(3000);
    MantidVec xValues;
    double delta;

    alg.setOptions(numBins, true, false);

    // first check that using zero for a border doesn't work
    TS_ASSERT_THROWS_ANYTHING(delta = alg.determineBinning(xValues, 0, 300));
    TS_ASSERT_THROWS_ANYTHING(delta = alg.determineBinning(xValues, -300, 0));

    // do an actual run
    delta = alg.determineBinning(xValues, 0.1, 1.0);
    //    std::cout << "**** " << numBins << " **** " << xValues.front() << ", "
    //    << delta << ", " << xValues.back() << " ****\n";
    //    std::cout << "000> " << xValues.size() << '\n';
    //    std::cout << "001> " << xValues[0] << ", " << xValues[1] << ", ..., "
    //    << xValues.back() << '\n';
    TS_ASSERT_EQUALS(numBins, xValues.size() - 1);
    TS_ASSERT_EQUALS(0.1, xValues[0]);
    TS_ASSERT_EQUALS(1., xValues[3000]);
    TS_ASSERT_DELTA(-.00077, delta, .00001);
  }

  void test_log_binning_density() {
    ResampleX alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());

    //
    int numBins(3000);
    MantidVec xValues;
    double delta;

    alg.setOptions(numBins, true, true);

    // first check that using zero for a border doesn't work
    TS_ASSERT_THROWS_ANYTHING(delta = alg.determineBinning(xValues, 0, 300));
    TS_ASSERT_THROWS_ANYTHING(delta = alg.determineBinning(xValues, -300, 0));

    // do an actual run
    delta = alg.determineBinning(xValues, 0.1, 1.0);
    //    std::cout << "**** " << numBins << " **** " << xValues.front() << ", "
    //    << delta << ", " << xValues.back() << " ****\n";
    //    std::cout << "000> " << xValues.size() << '\n';
    //    std::cout << "001> " << xValues[0] << ", " << xValues[1] << ", ..., "
    //    << xValues.back() << '\n';
    TS_ASSERT_EQUALS(numBins, xValues.size());
    TS_ASSERT_EQUALS(0.1, xValues[0]);
    TS_ASSERT_EQUALS(1., xValues[2999]);
    TS_ASSERT_DELTA(-.00077, delta, .00001);
  }

  void xtest_exec() {
    std::string outWSName("ResampleX_out");

    ResampleX alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());

    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InputWorkspace", "value"));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", outWSName));
    //    "XMin"
    //    "XMax"
    //    "NumberBins"
    //    "LogBinning"
    //    "PreserveEvents"
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // Retrieve the workspace from data service. TODO: Change to your desired
    // type
    Workspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING(
        ws = AnalysisDataService::Instance().retrieveWS<Workspace>(outWSName));
    TS_ASSERT(ws);
    if (!ws)
      return;

    // TODO: Check the results

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
  }

  // Set-up a generic function for running tests for EventWorkspace
  void do_testResampleXEventWorkspace(EventType eventType, bool inPlace,
                                      bool PreserveEvents,
                                      bool expectOutputEvent) {

    // Two events per bin
    EventWorkspace_sptr test_in =
        WorkspaceCreationHelper::createEventWorkspace2(2, 100);
    test_in->switchEventType(eventType);

    std::string inName("test_inEvent");
    std::string outName("test_inEvent_output");
    if (inPlace)
      outName = inName;

    AnalysisDataService::Instance().addOrReplace(inName, test_in);
    ResampleX alg;
    alg.initialize();
    alg.setPropertyValue("InputWorkspace", inName);
    alg.setPropertyValue("OutputWorkspace", outName);
    alg.setPropertyValue("Xmin", "0.0,0.0");
    alg.setPropertyValue("Xmax", "100,150");
    alg.setPropertyValue("NumberBins", "50");
    alg.setProperty("PreserveEvents", PreserveEvents);
    TS_ASSERT(alg.execute());
    TS_ASSERT(alg.isExecuted());

    MatrixWorkspace_sptr outWS;
    EventWorkspace_sptr eventOutWS;
    TS_ASSERT_THROWS_NOTHING(
        outWS = boost::dynamic_pointer_cast<MatrixWorkspace>(
            AnalysisDataService::Instance().retrieve(outName)));
    TS_ASSERT(outWS);
    if (!outWS)
      return;

    // Is the output gonna be events?
    if (expectOutputEvent) {
      eventOutWS = boost::dynamic_pointer_cast<EventWorkspace>(outWS);
      TS_ASSERT(eventOutWS);
      if (!eventOutWS)
        return;
      TS_ASSERT_EQUALS(eventOutWS->getNumberEvents(), 2 * 100 * 2);
      // Check that it is the same workspace
      if (inPlace)
        TS_ASSERT(eventOutWS == test_in);
    }

    // Define tolerance for ASSERT_DELTA
    double tolerance = 1.0e-10;

    // Check the first workspace index (from 0 to 100)
    auto &X1 = outWS->x(0);
    auto &Y1 = outWS->y(0);
    auto &E1 = outWS->e(0);

    TS_ASSERT_EQUALS(X1.size(), 51);
    TS_ASSERT_DELTA(X1[0], 0.0, tolerance);
    TS_ASSERT_DELTA(X1[1], 2.0, tolerance);
    TS_ASSERT_DELTA(X1[2], 4.0, tolerance);

    TS_ASSERT_EQUALS(Y1.size(), 50);
    TS_ASSERT_DELTA(Y1[0], 4.0, tolerance);
    TS_ASSERT_DELTA(Y1[1], 4.0, tolerance);
    TS_ASSERT_DELTA(Y1[2], 4.0, tolerance);

    TS_ASSERT_EQUALS(E1.size(), 50);
    TS_ASSERT_DELTA(E1[0], sqrt(4.0), tolerance);
    TS_ASSERT_DELTA(E1[1], sqrt(4.0), tolerance);

    // Check the second workspace index (from 0 to 150)
    auto &X2 = outWS->x(1);
    auto &Y2 = outWS->y(1);
    auto &E2 = outWS->e(1);

    TS_ASSERT_EQUALS(X2.size(), 51);
    TS_ASSERT_DELTA(X2[0], 0.0, tolerance);
    TS_ASSERT_DELTA(X2[1], 3.0, tolerance);
    TS_ASSERT_DELTA(X2[2], 6.0, tolerance);

    TS_ASSERT_EQUALS(Y2.size(), 50);
    TS_ASSERT_DELTA(Y2[0], 6.0, tolerance);
    TS_ASSERT_DELTA(Y2[1], 6.0, tolerance);
    TS_ASSERT_DELTA(Y2[2], 6.0, tolerance);

    TS_ASSERT_EQUALS(E2.size(), 50);
    TS_ASSERT_DELTA(E2[0], sqrt(6.0), tolerance);
    TS_ASSERT_DELTA(E2[1], sqrt(6.0), tolerance);

    AnalysisDataService::Instance().remove(inName);
    AnalysisDataService::Instance().remove(outName);
  }

  void testEventWorkspace_InPlace_PreserveEvents() {
    do_testResampleXEventWorkspace(TOF, true, true, true);
  }

  void testEventWorkspace_InPlace_PreserveEvents_weighted() {
    do_testResampleXEventWorkspace(WEIGHTED, true, true, true);
  }

  void testEventWorkspace_InPlace_PreserveEvents_weightedNoTime() {
    do_testResampleXEventWorkspace(WEIGHTED_NOTIME, true, true, true);
  }

  void testEventWorkspace_InPlace_NoPreserveEvents() {
    do_testResampleXEventWorkspace(TOF, true, false, false);
  }

  void testEventWorkspace_InPlace_NoPreserveEvents_weighted() {
    do_testResampleXEventWorkspace(WEIGHTED, true, false, false);
  }

  void testEventWorkspace_InPlace_NoPreserveEvents_weightedNoTime() {
    do_testResampleXEventWorkspace(WEIGHTED_NOTIME, true, false, false);
  }

  void testEventWorkspace_NotInPlace_NoPreserveEvents() {
    do_testResampleXEventWorkspace(TOF, false, false, false);
  }

  void testEventWorkspace_NotInPlace_NoPreserveEvents_weighted() {
    do_testResampleXEventWorkspace(WEIGHTED, false, false, false);
  }

  void testEventWorkspace_NotInPlace_NoPreserveEvents_weightedNoTime() {
    do_testResampleXEventWorkspace(WEIGHTED_NOTIME, false, false, false);
  }

  void testEventWorkspace_NotInPlace_PreserveEvents() {
    do_testResampleXEventWorkspace(TOF, false, true, true);
  }

  void testEventWorkspace_NotInPlace_PreserveEvents_weighted() {
    do_testResampleXEventWorkspace(WEIGHTED, false, true, true);
  }

  void testEventWorkspace_NotInPlace_PreserveEvents_weightedNoTime() {
    do_testResampleXEventWorkspace(WEIGHTED_NOTIME, false, true, true);
  }

  // Set-up a generic function for running tests for Workspace2D
  void do_testResampleXWorkspace2D(bool inPlace, bool withDistribution) {

    std::string inName("test_inEvent");
    std::string outName("test_inEvent_output");
    if (inPlace)
      outName = inName;

    Workspace2D_sptr test_in2D = Create2DWorkspace(100, 2);
    test_in2D->setDistribution(withDistribution);
    AnalysisDataService::Instance().add(inName, test_in2D);

    ResampleX alg;
    alg.initialize();
    alg.setPropertyValue("InputWorkspace", inName);
    alg.setPropertyValue("OutputWorkspace", outName);
    alg.setPropertyValue("Xmin", "0.0,0.0");
    alg.setPropertyValue("Xmax", "100,150");
    alg.setPropertyValue("NumberBins", "50");

    TS_ASSERT(alg.execute());
    TS_ASSERT(alg.isExecuted());

    MatrixWorkspace_sptr outWS;
    EventWorkspace_sptr eventOutWS;
    TS_ASSERT_THROWS_NOTHING(
        outWS = boost::dynamic_pointer_cast<MatrixWorkspace>(
            AnalysisDataService::Instance().retrieve(outName)));
    TS_ASSERT(outWS);
    if (!outWS)
      return;

    // Define tolerance for ASSERT_DELTA
    double tolerance = 1.0e-10;

    // Check the first workspace index (from 0 to 100)
    auto &X1 = outWS->x(0);
    auto &Y1 = outWS->y(0);
    auto &E1 = outWS->e(0);

    TS_ASSERT_EQUALS(X1.size(), 51);
    TS_ASSERT_DELTA(X1[0], 0.0, tolerance);
    TS_ASSERT_DELTA(X1[1], 2.0, tolerance);
    TS_ASSERT_DELTA(X1[2], 4.0, tolerance);

    TS_ASSERT_EQUALS(Y1.size(), 50);
    TS_ASSERT_DELTA(Y1[0], 6.0, tolerance);
    TS_ASSERT_DELTA(Y1[1], 8.0, tolerance);
    TS_ASSERT_DELTA(Y1[2], 8.0, tolerance);

    TS_ASSERT_EQUALS(E1.size(), 50);
    TS_ASSERT_DELTA(E1[0], sqrt(6.0), tolerance);
    TS_ASSERT_DELTA(E1[1], sqrt(8.0), tolerance);

    // Check the second workspace index (from 0 to 150)
    auto &X2 = outWS->x(1);
    auto &Y2 = outWS->y(1);
    auto &E2 = outWS->e(1);

    TS_ASSERT_EQUALS(X2.size(), 51);
    TS_ASSERT_DELTA(X2[0], 0.0, tolerance);
    TS_ASSERT_DELTA(X2[1], 3.0, tolerance);
    TS_ASSERT_DELTA(X2[2], 6.0, tolerance);

    TS_ASSERT_EQUALS(Y2.size(), 50);
    TS_ASSERT_DELTA(Y2[0], 10.0, tolerance);
    TS_ASSERT_DELTA(Y2[1], 12.0, tolerance);
    TS_ASSERT_DELTA(Y2[2], 12.0, tolerance);

    TS_ASSERT_EQUALS(E2.size(), 50);
    TS_ASSERT_DELTA(E2[0], sqrt(10.0), tolerance);
    TS_ASSERT_DELTA(E2[1], sqrt(12.0), tolerance);

    AnalysisDataService::Instance().remove(inName);
    AnalysisDataService::Instance().remove(outName);
  }

  void testWorkspace2D_InPlace_NoDistribution() {
    do_testResampleXWorkspace2D(true, false);
  }

  void testWorkspace2D_NotInPlace_NoDistribution() {
    do_testResampleXWorkspace2D(false, false);
  }

  void xtestWorkspace2D_InPlace_WithDistribution() {
    do_testResampleXWorkspace2D(true, true);
  }

  void xtestWorkspace2D_NotInPlace_WithDistribution() {
    do_testResampleXWorkspace2D(false, true);
  }

private:
  // Function to create 2D Workspace, copied from RebinTest.h
  Workspace2D_sptr Create2DWorkspace(int xlen, int ylen) {
    BinEdges x1(xlen, HistogramData::LinearGenerator(0.5, 0.75));
    Counts y1(xlen - 1, 3.0);
    CountStandardDeviations e1(xlen - 1, sqrt(3.0));

    auto retVal = createWorkspace<Workspace2D>(ylen, xlen, xlen - 1);

    for (int i = 0; i < ylen; i++) {
      retVal->setBinEdges(i, x1);
      retVal->setCounts(i, y1);
      retVal->setCountStandardDeviations(i, e1);
    }

    return retVal;
  }
};

#endif /* MANTID_ALGORITHMS_REBINRAGGEDTEST_H_ */
