#ifndef INTEGRATIONTEST_H_
#define INTEGRATIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAlgorithms/Integration.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/RebinnedOutput.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/IDTypes.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <MantidHistogramData/LinearGenerator.h>
#include <MantidHistogramData/Points.h>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Algorithms;
using namespace Mantid::DataObjects;
using namespace Mantid::HistogramData;
using Mantid::MantidVec;
using Mantid::specnum_t;

class IntegrationTest : public CxxTest::TestSuite {
public:
  static IntegrationTest *createSuite() { return new IntegrationTest(); }
  static void destroySuite(IntegrationTest *suite) { delete suite; }

  IntegrationTest() {
    // Set up a small workspace for testing
    Workspace_sptr space =
        WorkspaceFactory::Instance().create("Workspace2D", 5, 6, 5);
    Workspace2D_sptr space2D = boost::dynamic_pointer_cast<Workspace2D>(space);
    double *a = new double[25];
    double *e = new double[25];
    for (int i = 0; i < 25; ++i) {
      a[i] = i;
      e[i] = sqrt(double(i));
    }
    for (int j = 0; j < 5; ++j) {
      auto &X = space2D->mutableX(j);

      for (int k = 0; k < 6; ++k) {
        X[k] = k;
      }
      space2D->mutableY(j).assign(a + (5 * j), a + (5 * j) + 5);
      space2D->mutableE(j).assign(e + (5 * j), e + (5 * j) + 5);
    }
    // Register the workspace in the data service
    AnalysisDataService::Instance().add("testSpace", space);
  }

  ~IntegrationTest() override { AnalysisDataService::Instance().clear(); }

  void testInit() {
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());

    // Set the properties
    alg.setPropertyValue("InputWorkspace", "testSpace");
    outputSpace = "IntegrationOuter";
    alg.setPropertyValue("OutputWorkspace", outputSpace);

    alg.setPropertyValue("RangeLower", "0.1");
    alg.setPropertyValue("RangeUpper", "4.0");
    alg.setPropertyValue("StartWorkspaceIndex", "2");
    alg.setPropertyValue("EndWorkspaceIndex", "4");

    TS_ASSERT_THROWS_NOTHING(alg2.initialize());
    TS_ASSERT(alg2.isInitialized());

    // Set the properties
    alg2.setPropertyValue("InputWorkspace", "testSpace");
    alg2.setPropertyValue("OutputWorkspace", "out2");

    TS_ASSERT_THROWS_NOTHING(alg3.initialize());
    TS_ASSERT(alg3.isInitialized());

    // Set the properties
    alg3.setPropertyValue("InputWorkspace", "testSpace");
    alg3.setPropertyValue("OutputWorkspace", "out3");
    alg3.setPropertyValue("RangeLower", "0.1");
    alg3.setPropertyValue("RangeUpper", "4.5");
    alg3.setPropertyValue("StartWorkspaceIndex", "2");
    alg3.setPropertyValue("EndWorkspaceIndex", "4");
    alg3.setPropertyValue("IncludePartialBins", "1");
  }

  void testNoCrashInside1Bin() {
    TS_ASSERT_THROWS_NOTHING(algNoCrash.initialize());
    TS_ASSERT(algNoCrash.isInitialized());
    // Set the properties
    algNoCrash.setPropertyValue("InputWorkspace", "testSpace");
    algNoCrash.setPropertyValue("OutputWorkspace", "outNoCrash");
    algNoCrash.setPropertyValue("RangeLower", "1.1");
    algNoCrash.setPropertyValue("RangeUpper", "1.3");
    TS_ASSERT_THROWS_NOTHING(algNoCrash.execute());
    TS_ASSERT(algNoCrash.isExecuted());
    AnalysisDataService::Instance().remove("outNoCrash");
  }

  void testRangeNoPartialBins() {
    if (!alg.isInitialized())
      alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    // Get back the saved workspace
    Workspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(
        output = AnalysisDataService::Instance().retrieve(outputSpace));

    Workspace2D_sptr output2D =
        boost::dynamic_pointer_cast<Workspace2D>(output);
    size_t max = 0;
    TS_ASSERT_EQUALS(max = output2D->getNumberHistograms(), 3);
    double yy[3] = {36, 51, 66};
    for (size_t i = 0; i < max; ++i) {
      const auto &x = output2D->x(i);
      const auto &y = output2D->y(i);
      const auto &e = output2D->e(i);

      TS_ASSERT_EQUALS(x.size(), 2);
      TS_ASSERT_EQUALS(y.size(), 1);
      TS_ASSERT_EQUALS(e.size(), 1);

      TS_ASSERT_EQUALS(x[0], 1.0);
      TS_ASSERT_EQUALS(x[1], 4.0);
      TS_ASSERT_EQUALS(y[0], yy[i]);
      TS_ASSERT_DELTA(e[0], sqrt(yy[i]), 0.001);
    }
  }

  void testNoRangeNoPartialBins() {
    if (!alg2.isInitialized())
      alg2.initialize();

    // Check setting of invalid property value causes failure
    TS_ASSERT_THROWS(alg2.setPropertyValue("StartWorkspaceIndex", "-1"),
                     std::invalid_argument);

    TS_ASSERT_THROWS_NOTHING(alg2.execute());
    TS_ASSERT(alg2.isExecuted());

    // Get back the saved workspace
    Workspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(
        output = AnalysisDataService::Instance().retrieve("out2"));

    Workspace2D_sptr output2D =
        boost::dynamic_pointer_cast<Workspace2D>(output);
    TS_ASSERT_EQUALS(output2D->getNumberHistograms(), 5);
    TS_ASSERT_EQUALS(output2D->x(0)[0], 0);
    TS_ASSERT_EQUALS(output2D->x(0)[1], 5);
    TS_ASSERT_EQUALS(output2D->y(0)[0], 10);
    TS_ASSERT_EQUALS(output2D->y(4)[0], 110);
    TS_ASSERT_DELTA(output2D->e(2)[0], 7.746, 0.001);
  }

  void testRangeWithPartialBins() {
    if (!alg3.isInitialized())
      alg3.initialize();
    TS_ASSERT_THROWS_NOTHING(alg3.execute());
    TS_ASSERT(alg3.isExecuted());

    // Get back the saved workspace
    Workspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(
        output = AnalysisDataService::Instance().retrieve("out3"));

    Workspace2D_sptr output2D =
        boost::dynamic_pointer_cast<Workspace2D>(output);
    size_t max = 0;
    TS_ASSERT_EQUALS(max = output2D->getNumberHistograms(), 3);
    const double yy[3] = {52., 74., 96.};
    const double ee[3] = {6.899, 8.240, 9.391};
    for (size_t i = 0; i < max; ++i) {
      const auto &x = output2D->x(i);
      const auto &y = output2D->y(i);
      const auto &e = output2D->e(i);

      TS_ASSERT_EQUALS(x.size(), 2);
      TS_ASSERT_EQUALS(y.size(), 1);
      TS_ASSERT_EQUALS(e.size(), 1);

      TS_ASSERT_EQUALS(x[0], 0.1);
      TS_ASSERT_EQUALS(x[1], 4.5);
      TS_ASSERT_EQUALS(y[0], yy[i]);
      TS_ASSERT_DELTA(e[0], ee[i], 0.001);
    }

    // Test that the same values occur for a distribution
    Workspace_sptr input;
    TS_ASSERT_THROWS_NOTHING(
        input = AnalysisDataService::Instance().retrieve("testSpace"));
    Workspace2D_sptr input2D = boost::dynamic_pointer_cast<Workspace2D>(output);
    input2D->setDistribution(true);
    // Replace workspace
    AnalysisDataService::Instance().addOrReplace("testSpace", input2D);

    alg3.execute();
    // Retest
    TS_ASSERT_THROWS_NOTHING(
        output = AnalysisDataService::Instance().retrieve("out3"));

    output2D = boost::dynamic_pointer_cast<Workspace2D>(output);
    TS_ASSERT_EQUALS(max = output2D->getNumberHistograms(), 3);
    for (size_t i = 0; i < max; ++i) {
      const auto &x = output2D->x(i);
      const auto &y = output2D->y(i);
      const auto &e = output2D->e(i);

      TS_ASSERT_EQUALS(x.size(), 2);
      TS_ASSERT_EQUALS(y.size(), 1);
      TS_ASSERT_EQUALS(e.size(), 1);

      TS_ASSERT_EQUALS(x[0], 0.1);
      TS_ASSERT_EQUALS(x[1], 4.5);
      TS_ASSERT_EQUALS(y[0], yy[i]);
      TS_ASSERT_DELTA(e[0], ee[i], 0.001);
    }
  }

  void doTestEvent(std::string inName, std::string outName,
                   int StartWorkspaceIndex, int EndWorkspaceIndex) {
    int numPixels = 100;
    int numBins = 50;
    EventWorkspace_sptr inWS = WorkspaceCreationHelper::CreateEventWorkspace(
        numPixels, numBins, numBins, 0.0, 1.0, 2);
    AnalysisDataService::Instance().addOrReplace(inName, inWS);

    Integration integ;
    integ.initialize();
    integ.setPropertyValue("InputWorkspace", inName);
    integ.setPropertyValue("OutputWorkspace", outName);
    integ.setPropertyValue("RangeLower", "9.9");
    integ.setPropertyValue("RangeUpper", "20.1");
    integ.setProperty("StartWorkspaceIndex", StartWorkspaceIndex);
    integ.setProperty("EndWorkspaceIndex", EndWorkspaceIndex);

    integ.execute();
    TS_ASSERT(integ.isExecuted());

    // No longer output an EventWorkspace
    Workspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(
        output = AnalysisDataService::Instance().retrieve(outName));
    Workspace2D_sptr output2D =
        boost::dynamic_pointer_cast<Workspace2D>(output);

    // Check that it is a matrix workspace
    TS_ASSERT(output);
    if (!output)
      return;

    TS_ASSERT_EQUALS(output2D->getNumberHistograms(),
                     EndWorkspaceIndex - StartWorkspaceIndex + 1);

    for (size_t i = 0; i < output2D->getNumberHistograms(); i++) {
      const auto &X = output2D->x(i);
      const auto &Y = output2D->y(i);
      const auto &E = output2D->e(i);
      TS_ASSERT_EQUALS(X.size(), 2);
      TS_ASSERT_EQUALS(Y.size(), 1);
      TS_ASSERT_DELTA(Y[0], 20.0, 1e-6);
      TS_ASSERT_DELTA(E[0], sqrt(20.0), 1e-6);
      // Correct spectra etc?
      specnum_t specNo = output2D->getSpectrum(i).getSpectrumNo();
      TS_ASSERT_EQUALS(specNo, StartWorkspaceIndex + i);
      TS_ASSERT(output2D->getSpectrum(i).hasDetectorID(specNo));
    }

    AnalysisDataService::Instance().remove(inName);
    AnalysisDataService::Instance().remove(outName);
  }

  void testEvent_AllHistograms() { doTestEvent("inWS", "outWS", 0, 99); }

  void testEvent_SomeHistograms() { doTestEvent("inWS", "outWS", 10, 39); }

  void testEvent_InPlace_AllHistograms() { doTestEvent("inWS", "inWS", 0, 99); }

  void testEvent_InPlace_SomeHistograms() {
    doTestEvent("inWS", "inWS", 10, 29);
  }

  void doTestRebinned(const std::string RangeLower,
                      const std::string RangeUpper,
                      const int StartWorkspaceIndex,
                      const int EndWorkspaceIndex,
                      const bool IncludePartialBins, const int expectedNumHists,
                      const double expectedVals[]) {
    RebinnedOutput_sptr inWS =
        WorkspaceCreationHelper::CreateRebinnedOutputWorkspace();
    std::string inName = inWS->getName();
    AnalysisDataService::Instance().addOrReplace(inName, inWS);
    std::string outName = "rebinInt";

    Integration integ;
    integ.initialize();
    integ.setPropertyValue("InputWorkspace", inName);
    integ.setPropertyValue("OutputWorkspace", outName);
    integ.setPropertyValue("RangeLower", RangeLower);
    integ.setPropertyValue("RangeUpper", RangeUpper);
    integ.setProperty("StartWorkspaceIndex", StartWorkspaceIndex);
    integ.setProperty("EndWorkspaceIndex", EndWorkspaceIndex);
    integ.setProperty("IncludePartialBins", IncludePartialBins);

    integ.execute();
    TS_ASSERT(integ.isExecuted());

    Workspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(
        output = AnalysisDataService::Instance().retrieve(outName));
    Workspace2D_sptr outputWS =
        boost::dynamic_pointer_cast<Workspace2D>(output);
    TS_ASSERT_EQUALS(outputWS->id(), "Workspace2D");

    double tol = 1.e-5;
    TS_ASSERT_EQUALS(outputWS->getNumberHistograms(), expectedNumHists);
    TS_ASSERT_DELTA(outputWS->y(1)[0], expectedVals[0], tol);
    TS_ASSERT_DELTA(outputWS->e(1)[0], expectedVals[1], tol);

    AnalysisDataService::Instance().remove(inName);
    AnalysisDataService::Instance().remove(outName);
  }

  void testRebinnedOutput_NoLimits() {
    const double truth[] = {6.0, 2.041241452319315};
    doTestRebinned("-3.0", "3.0", 0, 3, false, 4, truth);
  }

  void testRebinnedOutput_RangeLimits() {
    const double truth[] = {5.0, 1.9148542155126762};
    doTestRebinned("-2.0", "2.0", 0, 3, false, 4, truth);
  }

  void testRebinnedOutput_WorkspaceIndexLimits() {
    const double truth[] = {4.5, 1.8027756377319946};
    doTestRebinned("-3.0", "3.0", 1, 2, false, 2, truth);
  }

  void testRebinnedOutput_RangeLimitsWithPartialBins() {
    const double truth[] = {4.0, 1.4288690166235205};
    doTestRebinned("-1.5", "1.75", 0, 3, true, 4, truth);
  }

  void makeRealBinBoundariesWorkspace(const std::string inWsName) {
    const unsigned int lenX = 11, lenY = 10;

    // 1 difference, so it's holding BinEdges
    Workspace_sptr wsAsWs =
        WorkspaceFactory::Instance().create("Workspace2D", 1, lenX, lenY);
    Workspace2D_sptr ws = boost::dynamic_pointer_cast<Workspace2D>(wsAsWs);

    BinEdges x = {-1,  -0.8, -0.6, -0.4, -0.2, -2.22045e-16,
                  0.2, 0.4,  0.6,  0.8,  1};
    auto &xData = ws->mutableX(0);
    for (unsigned int i = 0; i < lenX; i++) {
      xData[i] = x[i];
      // Generate some rounding errors. Note: if you increase errors by making
      // this more complicated,
      // you'll eventually make Integration "fail".
      // Q is: how much tolerance should it have to inprecise numbers? For
      // example, replace the 13.3
      // multiplier/divisor by 13, and you'll get a -0.199999... sufficiently
      // different from the
      // initial -0.2 that Integration will fail to catch one bin and because of
      // that some tests will fail.
      xData[i] /= 2.5671;
      xData[i] *= 13.3;
      xData[i] /= 13.3;
      xData[i] *= 2.5671;
    }

    Counts y = {0, 0, 0, 2, 2, 2, 2, 0, 0, 0};
    ws->setCounts(0, y);

    auto &e = ws->mutableE(0);
    e.assign(e.size(), 0.0);

    AnalysisDataService::Instance().add(inWsName, ws);
  }

  void doTestRealBinBoundaries(const std::string inWsName,
                               const std::string rangeLower,
                               const std::string rangeUpper,
                               const double expectedVal,
                               const bool checkRanges = false,
                               const bool IncPartialBins = false) {
    Workspace_sptr auxWs;
    TS_ASSERT_THROWS_NOTHING(
        auxWs = AnalysisDataService::Instance().retrieve(inWsName));
    Workspace2D_sptr inWs = boost::dynamic_pointer_cast<Workspace2D>(auxWs);

    std::string outWsName = "out_real_boundaries_ws";

    Integration integ;
    integ.initialize();
    integ.setPropertyValue("InputWorkspace", inWs->getName());
    integ.setPropertyValue("OutputWorkspace", outWsName);
    integ.setPropertyValue("RangeLower", rangeLower);
    integ.setPropertyValue("RangeUpper", rangeUpper);
    integ.setProperty("IncludePartialBins", IncPartialBins);
    integ.execute();

    // should have created output work space
    TS_ASSERT_THROWS_NOTHING(
        auxWs = AnalysisDataService::Instance().retrieve(outWsName));
    Workspace2D_sptr outWs = boost::dynamic_pointer_cast<Workspace2D>(auxWs);
    TS_ASSERT_EQUALS(inWs->getNumberHistograms(), outWs->getNumberHistograms());

    if (checkRanges) {
      TS_ASSERT_LESS_THAN_EQUALS(atof(rangeLower.c_str()), outWs->x(0).front());
      TS_ASSERT_LESS_THAN_EQUALS(outWs->x(0).back(), atof(rangeUpper.c_str()));
    }
    // At last, check numerical results
    TS_ASSERT_DELTA(outWs->mutableY(0)[0], expectedVal, 1e-8);
  }

  void testProperHandlingOfIntegrationBoundaries() {
    std::string inWsName = "in_real_boundaries_ws";
    makeRealBinBoundariesWorkspace(inWsName);

    doTestRealBinBoundaries(inWsName, "-0.4", "-0.2", 2, true);
    doTestRealBinBoundaries(inWsName, "-0.2", "-0.0", 2, true);
    doTestRealBinBoundaries(inWsName, "-0.2", "0.2", 4, true);
    doTestRealBinBoundaries(inWsName, "-0.2", "0.4", 6, true);
    doTestRealBinBoundaries(inWsName, "-0.4", "0.2", 6, true);
    doTestRealBinBoundaries(inWsName, "-0.4", "0.4", 8, true);
    doTestRealBinBoundaries(inWsName, "-1", "1", 8, true);
    doTestRealBinBoundaries(inWsName, "-1.8", "1.2", 8, true);

    doTestRealBinBoundaries(inWsName, "-0.4", "-0.200001", 0, true);
    doTestRealBinBoundaries(inWsName, "-0.399999", "-0.2", 0, true);
    doTestRealBinBoundaries(inWsName, "-0.399999", "-0.200001", 0, true);
    doTestRealBinBoundaries(inWsName, "-0.3999", "-0.2", 0, true);

    doTestRealBinBoundaries(inWsName, "0.6", "6.5", 0, true);
    doTestRealBinBoundaries(inWsName, "-1", "-0.8", 0, true);
    doTestRealBinBoundaries(inWsName, "2.2", "3.03", 0);
    doTestRealBinBoundaries(inWsName, "-42.2", "-3.03", 0);
  }

  void test_point_data_linear_x() {
    // Set up a small workspace for testing
    const size_t nspec = 5;
    Workspace_sptr space =
        WorkspaceFactory::Instance().create("Workspace2D", nspec, 5, 5);
    Workspace2D_sptr space2D = boost::dynamic_pointer_cast<Workspace2D>(space);

    for (int j = 0; j < 5; ++j) {
      auto &e = space2D->mutableE(j);
      space2D->setPoints(j, 5, LinearGenerator(0, 0.9));       // assign X
      space2D->setCounts(j, 5, LinearGenerator(double(j), 2)); // assign Y
      e.assign(e.size(), 1.0);
    }

    const std::string outWsName = "IntegrationTest_PointData";
    Integration alg;
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", space2D);
    alg.setPropertyValue("OutputWorkspace", outWsName);
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    MatrixWorkspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(
        output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            outWsName));
    TS_ASSERT(output);
    if (!output)
      return;

    TS_ASSERT_EQUALS(output->getNumberHistograms(), 5);
    TS_ASSERT_EQUALS(output->blocksize(), 1);
    TS_ASSERT(output->isHistogramData());

    TS_ASSERT_DELTA(output->x(0).front(), -0.5 * 0.9, 1e-14);
    TS_ASSERT_DELTA(output->x(0).back(), 4.5 * 0.9, 1e-14);

    TS_ASSERT_DELTA(output->y(0)[0], 20 * 0.9, 1e-14);
    TS_ASSERT_DELTA(output->y(1)[0], 25 * 0.9, 1e-14);
    TS_ASSERT_DELTA(output->y(2)[0], 30 * 0.9, 1e-14);
    TS_ASSERT_DELTA(output->y(3)[0], 35 * 0.9, 1e-14);
    TS_ASSERT_DELTA(output->y(4)[0], 40 * 0.9, 1e-14);

    AnalysisDataService::Instance().remove(outWsName);
  }

  void test_point_data_non_linear_x() {
    // Set up a small workspace for testing
    const size_t nspec = 5;
    Workspace_sptr space =
        WorkspaceFactory::Instance().create("Workspace2D", nspec, 5, 5);
    Workspace2D_sptr space2D = boost::dynamic_pointer_cast<Workspace2D>(space);

    for (int j = 0; j < 5; ++j) {
      auto &x = space2D->mutableX(j);
      auto &e = space2D->mutableE(j);

      for (int k = 0; k < 5; ++k) {
        x[k] = k * (1.0 + 1.0 * k);
      }
      space2D->setCounts(j, 5, LinearGenerator(double(j), 2)); // assign Y
      e.assign(e.size(), 1.0);
    }

    const std::string outWsName = "IntegrationTest_PointData";
    Integration alg;
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", space2D);
    alg.setPropertyValue("OutputWorkspace", outWsName);
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    MatrixWorkspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(
        output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            outWsName));
    TS_ASSERT(output);
    if (!output)
      return;

    TS_ASSERT_EQUALS(output->getNumberHistograms(), 5);
    TS_ASSERT_EQUALS(output->blocksize(), 1);
    TS_ASSERT(output->isHistogramData());

    TS_ASSERT_EQUALS(output->x(0).front(), -1.0);
    TS_ASSERT_EQUALS(output->x(0).back(), 24.0);

    TS_ASSERT_EQUALS(output->y(0)[0], 132.0);
    TS_ASSERT_EQUALS(output->y(1)[0], 157.0);
    TS_ASSERT_EQUALS(output->y(2)[0], 182.0);
    TS_ASSERT_EQUALS(output->y(3)[0], 207.0);
    TS_ASSERT_EQUALS(output->y(4)[0], 232.0);

    AnalysisDataService::Instance().remove(outWsName);
  }

private:
  Integration alg;        // Test with range limits
  Integration alg2;       // Test without limits
  Integration alg3;       // Test with range and partial bins
  Integration algNoCrash; // test for integration inside bin
  std::string outputSpace;
};

#endif /*INTEGRATIONTEST_H_*/
