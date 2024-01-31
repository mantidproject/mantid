// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAlgorithms/Integration.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/RebinnedOutput.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidGeometry/IDTypes.h"
#include "MantidHistogramData/Histogram.h"
#include "MantidHistogramData/LinearGenerator.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Algorithms;
using namespace Mantid::DataObjects;
using Mantid::MantidVec;
using Mantid::specnum_t;

class IntegrationTest : public CxxTest::TestSuite {
public:
  static IntegrationTest *createSuite() { return new IntegrationTest(); }
  static void destroySuite(IntegrationTest *suite) { delete suite; }

  void setUp() override {
    using namespace Mantid::HistogramData;
    // Set up a small workspace for testing
    const size_t nhist(5), nbins(5);
    const bool isHist = true;
    auto space2D = WorkspaceCreationHelper::create2DWorkspace123(nhist, nbins, isHist);
    // replace values
    // X=0->nbins+1,Y=0->nhist*nbins
    auto xValues = make_cow<HistogramX>(nbins + 1, LinearGenerator(0., 1.0));
    for (size_t i = 0; i < nhist; ++i) {
      Counts counts(nbins, LinearGenerator(nbins * static_cast<double>(i), 1.0));
      space2D->setHistogram(i, Histogram(BinEdges(xValues), counts));
    }

    // Register the workspace in the data service
    AnalysisDataService::Instance().add("testSpace", space2D);
  }

  void tearDown() override { AnalysisDataService::Instance().clear(); }

  void testInit() {
    Integration alg;
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
  }

  void testNoCrashInside1Bin() {
    Integration algNoCrash;
    algNoCrash.setRethrows(true);
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
    Integration alg;
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    alg.setPropertyValue("InputWorkspace", "testSpace");
    const std::string outputSpace = "IntegrationOuter";
    alg.setPropertyValue("OutputWorkspace", outputSpace);
    alg.setPropertyValue("RangeLower", "0.1");
    alg.setPropertyValue("RangeUpper", "4.0");
    alg.setPropertyValue("StartWorkspaceIndex", "2");
    alg.setPropertyValue("EndWorkspaceIndex", "4");

    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    // Get back the saved workspace
    Workspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieve(outputSpace));

    Workspace2D_sptr output2D = std::dynamic_pointer_cast<Workspace2D>(output);
    size_t max = 0;
    TS_ASSERT_EQUALS(max = output2D->getNumberHistograms(), 3);
    double yy[3] = {36, 51, 66};
    for (size_t i = 0; i < max; ++i) {
      Mantid::MantidVec &x = output2D->dataX(i);
      Mantid::MantidVec &y = output2D->dataY(i);
      Mantid::MantidVec &e = output2D->dataE(i);

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
    Integration alg;
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());

    // Set the properties
    alg.setPropertyValue("InputWorkspace", "testSpace");
    alg.setPropertyValue("OutputWorkspace", "out2");

    // Check setting of invalid property value causes failure
    TS_ASSERT_THROWS(alg.setPropertyValue("StartWorkspaceIndex", "-1"), const std::invalid_argument &);

    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    // Get back the saved workspace
    Workspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieve("out2"));

    Workspace2D_sptr output2D = std::dynamic_pointer_cast<Workspace2D>(output);
    TS_ASSERT_EQUALS(output2D->getNumberHistograms(), 5);
    TS_ASSERT_EQUALS(output2D->dataX(0)[0], 0);
    TS_ASSERT_EQUALS(output2D->dataX(0)[1], 5);
    TS_ASSERT_EQUALS(output2D->dataY(0)[0], 10);
    TS_ASSERT_EQUALS(output2D->dataY(4)[0], 110);
    TS_ASSERT_DELTA(output2D->dataE(2)[0], 7.746, 0.001);
  }

  void testRangeWithPartialBins() {
    Workspace2D_sptr input;
    TS_ASSERT_THROWS_NOTHING(
        input = std::dynamic_pointer_cast<Workspace2D>(AnalysisDataService::Instance().retrieve("testSpace")))
    assertRangeWithPartialBins(input, 0.1, 4.5, {52., 74., 96.}, {6.899, 8.240, 9.391});
  }

  void testRangeWithPartialBinsAndDistributionData() {
    Workspace2D_sptr input;
    TS_ASSERT_THROWS_NOTHING(
        input = std::dynamic_pointer_cast<Workspace2D>(AnalysisDataService::Instance().retrieve("testSpace")))
    input->setDistribution(true);
    assertRangeWithPartialBins(input, 0.1, 4.5, {52., 74., 96.}, {6.899, 8.240, 9.391});
  }

  void testRangeWithPartialBinsAndLimitsInSameBin() {
    Workspace2D_sptr input;
    TS_ASSERT_THROWS_NOTHING(
        input = std::dynamic_pointer_cast<Workspace2D>(AnalysisDataService::Instance().retrieve("testSpace")))
    input->setDistribution(true);
    assertRangeWithPartialBins(input, 0.1, 0.5, {4., 6., 8.}, {1.265, 1.549, 1.788});
  }

  void testRangeWithPartialBinsAndEqualLimits() {
    Workspace2D_sptr input;
    TS_ASSERT_THROWS_NOTHING(
        input = std::dynamic_pointer_cast<Workspace2D>(AnalysisDataService::Instance().retrieve("testSpace")))
    input->setDistribution(true);
    assertRangeWithPartialBins(input, 0.1, 0.1, {0., 0., 0.}, {0., 0., 0.});
  }

  void doTestEvent(const std::string &inName, const std::string &outName, int StartWorkspaceIndex,
                   int EndWorkspaceIndex) {
    int numPixels = 100;
    int numBins = 50;
    EventWorkspace_sptr inWS = WorkspaceCreationHelper::createEventWorkspace(numPixels, numBins, numBins, 0.0, 1.0, 2);
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
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieve(outName));
    Workspace2D_sptr output2D = std::dynamic_pointer_cast<Workspace2D>(output);

    // Check that it is a matrix workspace
    TS_ASSERT(output);
    if (!output)
      return;

    TS_ASSERT_EQUALS(output2D->getNumberHistograms(), EndWorkspaceIndex - StartWorkspaceIndex + 1);

    for (size_t i = 0; i < output2D->getNumberHistograms(); i++) {
      MantidVec X = output2D->readX(i);
      MantidVec Y = output2D->readY(i);
      MantidVec E = output2D->readE(i);
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

  void testEvent_InPlace_SomeHistograms() { doTestEvent("inWS", "inWS", 10, 29); }

  void doTestRebinned(const std::string &RangeLower, const std::string &RangeUpper, const int StartWorkspaceIndex,
                      const int EndWorkspaceIndex, const bool IncludePartialBins, const int expectedNumHists,
                      const double expectedVals[]) {
    RebinnedOutput_sptr inWS = WorkspaceCreationHelper::createRebinnedOutputWorkspace();
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
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieve(outName));
    Workspace2D_sptr outputWS = std::dynamic_pointer_cast<Workspace2D>(output);
    TS_ASSERT_EQUALS(outputWS->id(), "RebinnedOutput");

    double tol = 1.e-5;
    TS_ASSERT_EQUALS(outputWS->getNumberHistograms(), expectedNumHists);
    TS_ASSERT_DELTA(outputWS->dataY(1)[0], expectedVals[0], tol);
    TS_ASSERT_DELTA(outputWS->dataE(1)[0], expectedVals[1], tol);

    AnalysisDataService::Instance().remove(inName);
    AnalysisDataService::Instance().remove(outName);
  }

  void testRebinnedOutput_NoLimits() {
    const double truth[] = {5.5, 1.6583123953};
    doTestRebinned("-3.0", "3.0", 0, 3, false, 4, truth);
  }

  void testRebinnedOutput_RangeLimits() {
    const double truth[] = {4.5, 1.5};
    doTestRebinned("-2.0", "2.0", 0, 3, false, 4, truth);
  }

  void testRebinnedOutput_WorkspaceIndexLimits() {
    const double truth[] = {4.6666666667, 1.7638342079};
    doTestRebinned("-3.0", "3.0", 1, 2, false, 2, truth);
  }

  void testRebinnedOutput_RangeLimitsWithPartialBins() {
    const double truth[] = {3.5862068967, 1.2173805720};
    doTestRebinned("-1.5", "1.75", 0, 3, true, 4, truth);
  }

  void testRebinnedOutput_FnorEquals0() {
    // Validate a bug fix that caused a divide by 0 error for some cases

    const double truth[] = {0, 0};
    doTestRebinned("-0.12", "0.12", 0, 3, false, 4, truth);
  }

  void makeRealBinBoundariesWorkspace(const std::string &inWsName) {
    const unsigned int lenX = 11, lenY = 10, lenE = lenY;

    Workspace_sptr wsAsWs = WorkspaceFactory::Instance().create("Workspace2D", 1, lenX, lenY);
    Workspace2D_sptr ws = std::dynamic_pointer_cast<Workspace2D>(wsAsWs);

    double x[lenX] = {-1, -0.8, -0.6, -0.4, -0.2, -2.22045e-16, 0.2, 0.4, 0.6, 0.8, 1};
    for (unsigned int i = 0; i < lenX; i++) {
      ws->dataX(0)[i] = x[i];
      // Generate some rounding errors. Note: if you increase errors by making
      // this more complicated,
      // you'll eventually make Integration "fail".
      // Q is: how much tolerance should it have to inprecise numbers? For
      // example, replace the 13.3
      // multiplier/divisor by 13, and you'll get a -0.199999... sufficiently
      // different from the
      // initial -0.2 that Integration will fail to catch one bin and because of
      // that some tests will fail.
      ws->dataX(0)[i] /= 2.5671;
      ws->dataX(0)[i] *= 13.3;
      ws->dataX(0)[i] /= 13.3;
      ws->dataX(0)[i] *= 2.5671;
    }
    double y[lenY] = {0, 0, 0, 2, 2, 2, 2, 0, 0, 0};
    for (unsigned int i = 0; i < lenY; i++) {
      ws->dataY(0)[i] = y[i];
    }
    double e[lenE] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    for (unsigned int i = 0; i < lenE; i++) {
      ws->dataE(0)[i] = e[i];
    }
    AnalysisDataService::Instance().add(inWsName, ws);
  }

  void doTestRealBinBoundaries(const std::string &inWsName, const std::string &rangeLower,
                               const std::string &rangeUpper, const double expectedVal, const bool checkRanges = false,
                               const bool IncPartialBins = false) {
    Workspace_sptr auxWs;
    TS_ASSERT_THROWS_NOTHING(auxWs = AnalysisDataService::Instance().retrieve(inWsName));
    Workspace2D_sptr inWs = std::dynamic_pointer_cast<Workspace2D>(auxWs);

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
    TS_ASSERT_THROWS_NOTHING(auxWs = AnalysisDataService::Instance().retrieve(outWsName));
    Workspace2D_sptr outWs = std::dynamic_pointer_cast<Workspace2D>(auxWs);
    TS_ASSERT_EQUALS(inWs->getNumberHistograms(), outWs->getNumberHistograms());

    if (checkRanges) {
      TS_ASSERT_LESS_THAN_EQUALS(std::stod(rangeLower), outWs->dataX(0).front());

      TS_ASSERT_LESS_THAN_EQUALS(outWs->dataX(0).back(), std::stod(rangeUpper));
    }
    // At last, check numerical results
    TS_ASSERT_DELTA(outWs->dataY(0)[0], expectedVal, 1e-8);
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
    Workspace_sptr space = WorkspaceFactory::Instance().create("Workspace2D", nspec, 5, 5);
    Workspace2D_sptr space2D = std::dynamic_pointer_cast<Workspace2D>(space);

    for (int j = 0; j < 5; ++j) {
      for (int k = 0; k < 5; ++k) {
        space2D->dataX(j)[k] = 0.9 * k;
        space2D->dataY(j)[k] = 2 * k + double(j);
        space2D->dataE(j)[k] = 1.0;
      }
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
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outWsName));
    TS_ASSERT(output);
    if (!output)
      return;

    TS_ASSERT_EQUALS(output->getNumberHistograms(), 5);
    TS_ASSERT_EQUALS(output->blocksize(), 1);
    TS_ASSERT(output->isHistogramData());

    TS_ASSERT_DELTA(output->readX(0).front(), -0.5 * 0.9, 1e-14);
    TS_ASSERT_DELTA(output->readX(0).back(), 4.5 * 0.9, 1e-14);

    TS_ASSERT_DELTA(output->readY(0)[0], 20 * 0.9, 1e-14);
    TS_ASSERT_DELTA(output->readY(1)[0], 25 * 0.9, 1e-14);
    TS_ASSERT_DELTA(output->readY(2)[0], 30 * 0.9, 1e-14);
    TS_ASSERT_DELTA(output->readY(3)[0], 35 * 0.9, 1e-14);
    TS_ASSERT_DELTA(output->readY(4)[0], 40 * 0.9, 1e-14);

    AnalysisDataService::Instance().remove(outWsName);
  }

  void test_point_data_non_linear_x() {
    // Set up a small workspace for testing
    const size_t nspec = 5;
    Workspace_sptr space = WorkspaceFactory::Instance().create("Workspace2D", nspec, 5, 5);
    Workspace2D_sptr space2D = std::dynamic_pointer_cast<Workspace2D>(space);

    for (int j = 0; j < 5; ++j) {
      for (int k = 0; k < 5; ++k) {
        space2D->dataX(j)[k] = k * (1.0 + 1.0 * k);
        space2D->dataY(j)[k] = 2 * k + double(j);
        space2D->dataE(j)[k] = 1.0;
      }
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
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outWsName));
    TS_ASSERT(output);
    if (!output)
      return;

    TS_ASSERT_EQUALS(output->getNumberHistograms(), 5);
    TS_ASSERT_EQUALS(output->blocksize(), 1);
    TS_ASSERT(output->isHistogramData());

    TS_ASSERT_EQUALS(output->readX(0).front(), -1.0);
    TS_ASSERT_EQUALS(output->readX(0).back(), 24.0);

    TS_ASSERT_EQUALS(output->readY(0)[0], 132.0);
    TS_ASSERT_EQUALS(output->readY(1)[0], 157.0);
    TS_ASSERT_EQUALS(output->readY(2)[0], 182.0);
    TS_ASSERT_EQUALS(output->readY(3)[0], 207.0);
    TS_ASSERT_EQUALS(output->readY(4)[0], 232.0);

    AnalysisDataService::Instance().remove(outWsName);
  }

  void testRangeLists() {
    const std::vector<double> lowerLimits{{1, 2, 1, 3, 2}};
    const std::vector<double> upperLimits{{4, 3, 3, 4, 3}};
    Integration alg;
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InputWorkspace", "testSpace"))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "out"))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("RangeLowerList", lowerLimits))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("RangeUpperList", upperLimits))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())
    MatrixWorkspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("out"));
    TS_ASSERT_EQUALS(output->getNumberHistograms(), 5)
    TS_ASSERT_EQUALS(output->blocksize(), 1)
    const std::array<double, 5> correctAnswers{{1 + 2 + 3, 7, 11 + 12, 18, 22}};
    for (size_t i = 0; i < output->getNumberHistograms(); ++i) {
      TS_ASSERT_EQUALS(output->x(i)[0], lowerLimits[i])
      TS_ASSERT_EQUALS(output->x(i)[1], upperLimits[i])
      TS_ASSERT_EQUALS(output->y(i)[0], correctAnswers[i])
      TS_ASSERT_EQUALS(output->e(i)[0], std::sqrt(correctAnswers[i]))
    }
  }

  void testFailureIfRangeLowerListGreaterThanRangeUpperList() {
    const std::vector<double> lowerLimits{{4, 3, 3, 4, 3}};
    const std::vector<double> upperLimits{{1, 2, 1, 3, 2}};
    Integration alg;
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InputWorkspace", "testSpace"))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "out"))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("RangeLowerList", lowerLimits))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("RangeUpperList", upperLimits))
    TS_ASSERT_THROWS_ANYTHING(alg.execute())
    TS_ASSERT(!alg.isExecuted())
  }

  void testRangeLowerListAndRangeUpper() {
    const std::vector<double> lowerLimits{{1, 2, 1, 3, 2}};
    const double upperLimit = 4;
    Integration alg;
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InputWorkspace", "testSpace"))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "out"))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("RangeLowerList", lowerLimits))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("RangeUpper", upperLimit))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())
    MatrixWorkspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("out"));
    TS_ASSERT_EQUALS(output->getNumberHistograms(), 5)
    TS_ASSERT_EQUALS(output->blocksize(), 1)
    const std::array<double, 5> correctAnswers{{1 + 2 + 3, 7 + 8, 11 + 12 + 13, 18, 22 + 23}};
    for (size_t i = 0; i < output->getNumberHistograms(); ++i) {
      TS_ASSERT_EQUALS(output->x(i)[0], lowerLimits[i])
      TS_ASSERT_EQUALS(output->x(i)[1], upperLimit)
      TS_ASSERT_EQUALS(output->y(i)[0], correctAnswers[i])
      TS_ASSERT_DELTA(output->e(i)[0], std::sqrt(correctAnswers[i]), 1e-7)
    }
  }

  void testFailureIfRangeLowerListGreaterThanRangeUpper() {
    const std::vector<double> lowerLimits{{1, 2, 1, 3, 2}};
    const double upperLimit = 1;
    Integration alg;
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InputWorkspace", "testSpace"))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "out"))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("RangeLowerList", lowerLimits))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("RangeUpper", upperLimit))
    TS_ASSERT_THROWS_ANYTHING(alg.execute())
    TS_ASSERT(!alg.isExecuted())
  }

  void testRangeLowerAndRangeUpperList() {
    const double lowerLimit = 1;
    const std::vector<double> upperLimits{{2, 4, 5, 3, 4}};
    Integration alg;
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InputWorkspace", "testSpace"))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "out"))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("RangeLower", lowerLimit))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("RangeUpperList", upperLimits))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())
    MatrixWorkspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("out"));
    TS_ASSERT_EQUALS(output->getNumberHistograms(), 5)
    TS_ASSERT_EQUALS(output->blocksize(), 1)
    const std::array<double, 5> correctAnswers{{1, 6 + 7 + 8, 11 + 12 + 13 + 14, 16 + 17, 21 + 22 + 23}};
    for (size_t i = 0; i < output->getNumberHistograms(); ++i) {
      TS_ASSERT_EQUALS(output->x(i)[0], lowerLimit)
      TS_ASSERT_EQUALS(output->x(i)[1], upperLimits[i])
      TS_ASSERT_EQUALS(output->y(i)[0], correctAnswers[i])
      TS_ASSERT_DELTA(output->e(i)[0], std::sqrt(correctAnswers[i]), 1e-7)
    }
  }

  void testFailureIfRangeLowerGreaterThanRangeUpperList() {
    const double lowerLimit = 3;
    const std::vector<double> upperLimits{{2, 4, 5, 3, 4}};
    Integration alg;
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InputWorkspace", "testSpace"))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "out"))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("RangeLower", lowerLimit))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("RangeUpperList", upperLimits))
    TS_ASSERT_THROWS_ANYTHING(alg.execute())
    TS_ASSERT(!alg.isExecuted())
  }

  void testRangeListsWithStartAndEndWorkspaceIndices() {
    const std::vector<double> lowerLimits{{2, 2}};
    const std::vector<double> upperLimits{{3, 3}};
    Integration alg;
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InputWorkspace", "testSpace"))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "out"))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("StartWorkspaceIndex", 1))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("EndWorkspaceIndex", 2))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("RangeLowerList", lowerLimits))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("RangeUpperList", upperLimits))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())
    MatrixWorkspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("out"));
    TS_ASSERT_EQUALS(output->getNumberHistograms(), 2)
    TS_ASSERT_EQUALS(output->blocksize(), 1)
    const std::array<double, 2> correctAnswers{{7, 12}};
    for (size_t i = 0; i < output->getNumberHistograms(); ++i) {
      TS_ASSERT_EQUALS(output->x(i)[0], lowerLimits[i])
      TS_ASSERT_EQUALS(output->x(i)[1], upperLimits[i])
      TS_ASSERT_EQUALS(output->y(i)[0], correctAnswers[i])
      TS_ASSERT_DELTA(output->e(i)[0], std::sqrt(correctAnswers[i]), 1e-7)
    }
  }

  void testMinimumXUsedIfRangeLowerNotGiven() {
    Integration alg;
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InputWorkspace", "testSpace"))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "out"))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("RangeUpper", 4.0))
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    // Get back the saved workspace
    Workspace2D_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieveWS<Workspace2D>("out"));
    TS_ASSERT_EQUALS(output->getNumberHistograms(), 5)
    TS_ASSERT_EQUALS(output->blocksize(), 1)
    const std::array<double, 5> correctAnswers{{6, 26, 46, 66, 86}};
    for (size_t i = 0; i < output->getNumberHistograms(); ++i) {
      TS_ASSERT_EQUALS(output->x(i)[0], 0)
      TS_ASSERT_EQUALS(output->x(i)[1], 4)
      TS_ASSERT_EQUALS(output->y(i)[0], correctAnswers[i])
      TS_ASSERT_DELTA(output->e(i)[0], std::sqrt(correctAnswers[i]), 1e-7)
    }
  }

  void testAllFourRangesGiven() {
    const double lowerLimit = 1;
    const double upperLimit = 4;
    const std::vector<double> lowerLimits{{0, 1, 2, 1, 2}};
    const std::vector<double> upperLimits{{2, 4, 5, 3, 4}};
    Integration alg;
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InputWorkspace", "testSpace"))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "out"))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("RangeLower", lowerLimit))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("RangeUpper", upperLimit))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("RangeLowerList", lowerLimits))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("RangeUpperList", upperLimits))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())
    MatrixWorkspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("out"));
    TS_ASSERT_EQUALS(output->getNumberHistograms(), 5)
    TS_ASSERT_EQUALS(output->blocksize(), 1)
    const std::array<double, 5> correctAnswers{{1, 6 + 7 + 8, 12 + 13, 16 + 17, 22 + 23}};
    for (size_t i = 0; i < output->getNumberHistograms(); ++i) {
      TS_ASSERT_EQUALS(output->x(i)[0], std::max(lowerLimit, lowerLimits[i]))
      TS_ASSERT_EQUALS(output->x(i)[1], std::min(upperLimit, upperLimits[i]))
      TS_ASSERT_EQUALS(output->y(i)[0], correctAnswers[i])
      TS_ASSERT_DELTA(output->e(i)[0], std::sqrt(correctAnswers[i]), 1e-7)
    }
  }

  template <typename F> void wsBoundsTest(const std::string &workspace, int startIndex, int endIndex, F boundsAssert) {
    MatrixWorkspace_sptr input;
    TS_ASSERT_THROWS_NOTHING(input = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(workspace));

    Integration alg;
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InputWorkspace", workspace));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "out"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("StartWorkspaceIndex", startIndex));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("EndWorkspaceIndex", endIndex));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());
    MatrixWorkspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("out"));
    boundsAssert(input, output, startIndex, endIndex);
  }

  void testStartWsIndexOutOfBounds() {
    auto boundsAssert = [](const MatrixWorkspace_sptr &, const MatrixWorkspace_sptr &output, int, int endIndex) {
      TS_ASSERT_EQUALS(output->getNumberHistograms(), endIndex + 1);
    };

    wsBoundsTest("testSpace", 100, 4, boundsAssert);
  }

  void testStartWSIndexGreaterThanEnd() {
    auto boundsAssert = [](const MatrixWorkspace_sptr &input, const MatrixWorkspace_sptr &output, int startIndex, int) {
      TS_ASSERT_EQUALS(output->getNumberHistograms(), input->getNumberHistograms() - startIndex);
    };

    wsBoundsTest("testSpace", 4, 2, boundsAssert);
  }

  void testStartWSIndexEqualsEnd() {
    auto boundsAssert = [](const MatrixWorkspace_sptr &, const MatrixWorkspace_sptr &output, int, int) {
      TS_ASSERT_EQUALS(output->getNumberHistograms(), 1);
    };

    wsBoundsTest("testSpace", 3, 3, boundsAssert);
  }

  void testNegativeWsBounds() {
    const int startIndex = -1;
    const int endIndex = -2;

    MatrixWorkspace_sptr input;
    TS_ASSERT_THROWS_NOTHING(input = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("testSpace"));

    Integration alg;
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InputWorkspace", "testSpace"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "out"));
    TS_ASSERT_THROWS_ANYTHING(alg.setProperty("StartWorkspaceIndex", startIndex));
    TS_ASSERT_THROWS_ANYTHING(alg.setProperty("EndWorkspaceIndex", endIndex));
  }

private:
  void assertRangeWithPartialBins(const Workspace_sptr &input, const double rangeLower, const double rangeUpper,
                                  const std::array<double, 3> yy, const std::array<double, 3> ee) {
    Integration alg;
    alg.setRethrows(false);
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());

    // Set the properties
    alg.setProperty("InputWorkspace", input);
    alg.setPropertyValue("OutputWorkspace", "out");
    alg.setProperty("RangeLower", rangeLower);
    alg.setProperty("RangeUpper", rangeUpper);
    alg.setPropertyValue("StartWorkspaceIndex", "2");
    alg.setPropertyValue("EndWorkspaceIndex", "4");
    alg.setPropertyValue("IncludePartialBins", "1");
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    // Get back the saved workspace
    Workspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieve("out"));

    Workspace2D_sptr output2D = std::dynamic_pointer_cast<Workspace2D>(output);
    size_t max = 0;
    TS_ASSERT_EQUALS(max = output2D->getNumberHistograms(), 3);

    for (size_t i = 0; i < max; ++i) {
      Mantid::MantidVec &x = output2D->dataX(i);
      Mantid::MantidVec &y = output2D->dataY(i);
      Mantid::MantidVec &e = output2D->dataE(i);

      TS_ASSERT_EQUALS(x.size(), 2);
      TS_ASSERT_EQUALS(y.size(), 1);
      TS_ASSERT_EQUALS(e.size(), 1);

      TS_ASSERT_EQUALS(x[0], rangeLower);
      TS_ASSERT_EQUALS(x[1], rangeUpper);
      TS_ASSERT_EQUALS(y[0], yy[i]);
      TS_ASSERT_DELTA(e[0], ee[i], 0.001);
    }
  }
};
