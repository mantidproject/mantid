#ifndef MANTID_ALGORITHMS_REBINRAGGEDTEST_H_
#define MANTID_ALGORITHMS_REBINRAGGEDTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAlgorithms/CreateWorkspace.h"
#include "MantidAlgorithms/ResampleX.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidHistogramData/LinearGenerator.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using Mantid::Algorithms::ResampleX;
using Mantid::HistogramData::BinEdges;
using Mantid::HistogramData::CountStandardDeviations;
using Mantid::HistogramData::Counts;
using Mantid::HistogramData::Histogram;
using Mantid::MantidVec;
using std::vector;

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

  // Set-up a generic function for running tests with EventWorkspace
  void do_testResampleXEventWorkspace(EventType eventType, bool inPlace,
                                      bool PreserveEvents) {

    int xlen = 100;
    int ylen = 2;

    EventWorkspace_sptr test_in =
        WorkspaceCreationHelper::createEventWorkspace2(ylen, xlen);
    test_in->switchEventType(eventType);

    std::string inName("test_inEvent");
    std::string outName("test_inEvent_output");
    if (inPlace)
      outName = inName;

    AnalysisDataService::Instance().addOrReplace(inName, test_in);

    // Create and run the algorithm
    ResampleX alg;
    alg.initialize();
    alg.setPropertyValue("InputWorkspace", inName);
    alg.setPropertyValue("OutputWorkspace", outName);
    alg.setPropertyValue("Xmin", "0.0,0.0");
    alg.setPropertyValue("Xmax", "100,50");
    alg.setPropertyValue("NumberBins", "50");
    alg.setProperty("PreserveEvents", PreserveEvents);
    TS_ASSERT(alg.execute());
    TS_ASSERT(alg.isExecuted());

    MatrixWorkspace_sptr outWS;
    TS_ASSERT_THROWS_NOTHING(
        outWS = boost::dynamic_pointer_cast<MatrixWorkspace>(
            AnalysisDataService::Instance().retrieve(outName)));
    TS_ASSERT(outWS);
    if (!outWS)
      return;

    // Retrieve the Xmin and Xmax values into a vector
    vector<double> xmins = alg.getProperty("XMin");
    vector<double> xmaxs = alg.getProperty("XMax");
    int nBins = alg.getProperty("NumberBins");
    double deltaBin;

    // Define tolerance for ASSERT_DELTA
    double tolerance = 1.0e-10;

    // Loop over spectra
    for (int yIndex = 0; yIndex < ylen; ++yIndex) {

      // The bin width for the current spectrum
      deltaBin = (xmaxs[yIndex] - xmins[yIndex]) / static_cast<double>(nBins);

      // Check the axes lengths
      TS_ASSERT_EQUALS(outWS->x(yIndex).size(), nBins + 1);
      TS_ASSERT_EQUALS(outWS->y(yIndex).size(), nBins);
      TS_ASSERT_EQUALS(outWS->e(yIndex).size(), nBins);

      // Loop over bins
      for (int xIndex = 0; xIndex < nBins; ++xIndex) {

        TS_ASSERT_DELTA((outWS->x(yIndex))[xIndex],
                        xmins[yIndex] + xIndex * deltaBin, tolerance);
        TS_ASSERT_DELTA((outWS->y(yIndex))[xIndex], xmaxs[yIndex] / 25.0,
                        tolerance);
        TS_ASSERT_DELTA((outWS->e(yIndex))[xIndex], sqrt(xmaxs[yIndex] / 25.0),
                        tolerance);
      }
    }

    AnalysisDataService::Instance().remove(inName);
    AnalysisDataService::Instance().remove(outName);
  }

  void testEventWorkspace_InPlace_PreserveEvents() {
    do_testResampleXEventWorkspace(TOF, true, true);
  }

  void testEventWorkspace_InPlace_PreserveEvents_weighted() {
    do_testResampleXEventWorkspace(WEIGHTED, true, true);
  }

  void testEventWorkspace_InPlace_PreserveEvents_weightedNoTime() {
    do_testResampleXEventWorkspace(WEIGHTED_NOTIME, true, true);
  }

  void testEventWorkspace_InPlace_NoPreserveEvents() {
    do_testResampleXEventWorkspace(TOF, true, false);
  }

  void testEventWorkspace_InPlace_NoPreserveEvents_weighted() {
    do_testResampleXEventWorkspace(WEIGHTED, true, false);
  }

  void testEventWorkspace_InPlace_NoPreserveEvents_weightedNoTime() {
    do_testResampleXEventWorkspace(WEIGHTED_NOTIME, true, false);
  }

  void testEventWorkspace_NotInPlace_NoPreserveEvents() {
    do_testResampleXEventWorkspace(TOF, false, false);
  }

  void testEventWorkspace_NotInPlace_NoPreserveEvents_weighted() {
    do_testResampleXEventWorkspace(WEIGHTED, false, false);
  }

  void testEventWorkspace_NotInPlace_NoPreserveEvents_weightedNoTime() {
    do_testResampleXEventWorkspace(WEIGHTED_NOTIME, false, false);
  }

  void testEventWorkspace_NotInPlace_PreserveEvents() {
    do_testResampleXEventWorkspace(TOF, false, true);
  }

  void testEventWorkspace_NotInPlace_PreserveEvents_weighted() {
    do_testResampleXEventWorkspace(WEIGHTED, false, true);
  }

  void testEventWorkspace_NotInPlace_PreserveEvents_weightedNoTime() {
    do_testResampleXEventWorkspace(WEIGHTED_NOTIME, false, true);
  }

  // Set-up a generic function for running tests with Workspace2D
  void do_testResampleXWorkspace2D(bool inPlace, bool withDistribution) {

    std::string inName("test_inEvent");
    std::string outName("test_inEvent_output");
    if (inPlace)
      outName = inName;

    int xlen = 100;
    int ylen = 2;
    double deltax = 0.75;
    double countVal = 3.0;

    Workspace2D_sptr ws = create<Workspace2D>(
        ylen, Histogram(BinEdges(xlen + 1,
                                 HistogramData::LinearGenerator(0.5, deltax)),
                        Counts(xlen, countVal)));
    ws->setDistribution(withDistribution);
    AnalysisDataService::Instance().add(inName, ws);

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
    TS_ASSERT_THROWS_NOTHING(
        outWS = boost::dynamic_pointer_cast<MatrixWorkspace>(
            AnalysisDataService::Instance().retrieve(outName)));
    TS_ASSERT(outWS);
    if (!outWS)
      return;

    // Retrieve the Xmin and Xmax values into an array
    vector<double> xmins = alg.getProperty("XMin");
    vector<double> xmaxs = alg.getProperty("XMax");
    int nBins = alg.getProperty("NumberBins");
    double deltaBin;

    // Define tolerance for ASSERT_DELTA
    double tolerance = 1.0e-10;
    double expectedCounts = 0.0;

    // Loop over spectra
    for (int yIndex = 0; yIndex < ylen; ++yIndex) {

      // The bin width for the current spectrum
      deltaBin = (xmaxs[yIndex] - xmins[yIndex]) / static_cast<double>(nBins);

      // Check the axes lengths
      TS_ASSERT_EQUALS(outWS->x(yIndex).size(), nBins + 1);
      TS_ASSERT_EQUALS(outWS->y(yIndex).size(), nBins);
      TS_ASSERT_EQUALS(outWS->e(yIndex).size(), nBins);

      // Loop over bins
      for (int xIndex = 0; xIndex < nBins; ++xIndex) {

        TS_ASSERT_DELTA((outWS->x(yIndex))[xIndex],
                        xmins[yIndex] + xIndex * deltaBin, tolerance);

        expectedCounts = -1.0;
        if (((outWS->x(yIndex))[xIndex] > (ws->x(yIndex))[0]) &&
            ((outWS->x(yIndex))[xIndex + 1] < (ws->x(yIndex))[xlen])) {
          expectedCounts = countVal * deltaBin / deltax;
        } else if (((outWS->x(yIndex))[xIndex] < (ws->x(yIndex))[0]) &&
                   ((outWS->x(yIndex))[xIndex + 1] < (ws->x(yIndex))[0])) {
          expectedCounts = 0.0;
        } else if (((outWS->x(yIndex))[xIndex] > (ws->x(yIndex))[xlen]) &&
                   ((outWS->x(yIndex))[xIndex + 1] > (ws->x(yIndex))[xlen])) {
          expectedCounts = 0.0;
        }

        if (expectedCounts > -0.5) {
          TS_ASSERT_DELTA((outWS->y(yIndex))[xIndex], expectedCounts,
                          tolerance);
          TS_ASSERT_DELTA((outWS->e(yIndex))[xIndex], sqrt(expectedCounts),
                          tolerance);
        }
      }
    }

    AnalysisDataService::Instance().remove(inName);
    AnalysisDataService::Instance().remove(outName);
  }

  void testWorkspace2D_InPlace_NoDistribution() {
    do_testResampleXWorkspace2D(true, false);
  }

  void xtestWorkspace2D_NotInPlace_NoDistribution() {
    do_testResampleXWorkspace2D(false, false);
  }

  // This test is disabled because ResampleX currently fails with distribution
  // data. See #22562
  void xtestWorkspace2D_InPlace_WithDistribution() {
    do_testResampleXWorkspace2D(true, true);
  }

  // This test is disabled because ResampleX currently fails with distribution
  // data. See #22562
  void xtestWorkspace2D_NotInPlace_WithDistribution() {
    do_testResampleXWorkspace2D(false, true);
  }
};

#endif /* MANTID_ALGORITHMS_REBINRAGGEDTEST_H_ */
