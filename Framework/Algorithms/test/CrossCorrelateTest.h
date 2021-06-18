// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/Expression.h"
#include "MantidAPI/IBackgroundFunction.h"
#include "MantidAPI/IConstraint.h"
#include "MantidAPI/ParamFunction.h"
#include "MantidAlgorithms/CrossCorrelate.h"
#include "MantidHistogramData/LinearGenerator.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include "CrossCorrelateTestData.h"

using namespace Mantid;
using namespace Mantid::Algorithms;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using Mantid::HistogramData::BinEdges;
using Mantid::HistogramData::CountStandardDeviations;

class CrossCorrelateTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being
  // created statically This means the constructor isn't called when
  // running other tests
  static CrossCorrelateTest *createSuite() { return new CrossCorrelateTest(); }
  static void destroySuite(CrossCorrelateTest *suite) { delete suite; }

  void testValidInput() {
    // setup and run the algorithm (includes basic checks)
    CrossCorrelate alg;
    const MatrixWorkspace_const_sptr inWS = setupAlgorithm(alg, 2.0, 4.0);
    const MatrixWorkspace_const_sptr outWS = runAlgorithm(alg, inWS);

    // specific checks
    const MantidVec &outX = outWS->readX(0);
    TS_ASSERT_EQUALS(outX.size(), 3);
    TS_ASSERT_DELTA(outX[0], -1.0, 1e-6);
    TS_ASSERT_DELTA(outX[1], 0.0, 1e-6);
    TS_ASSERT_DELTA(outX[2], 1.0, 1e-6);

    const MantidVec &outY0 = outWS->readY(0);
    TS_ASSERT_EQUALS(outY0.size(), 3);
    TS_ASSERT_DELTA(outY0[0], -0.018902, 1e-6);
    TS_ASSERT_DELTA(outY0[1], 1.0, 1e-6);
    TS_ASSERT_DELTA(outY0[2], -0.018902, 1e-6);

    const MantidVec &outY1 = outWS->readY(1);
    TS_ASSERT_EQUALS(outY1.size(), 3);
    TS_ASSERT_DELTA(outY1[0], -0.681363, 1e-6);
    TS_ASSERT_DELTA(outY1[1], 0.168384, 1e-6);
    TS_ASSERT_DELTA(outY1[2], 0.456851, 1e-6);
  }

  // This tests an input X length of 3, which is the minimum the algorithm can
  // handle
  void testMinimumInputXLength() {
    // setup and run the algorithm (includes basic checks)
    CrossCorrelate alg;
    const MatrixWorkspace_const_sptr inWS = setupAlgorithm(alg, 2.0, 3.5);
    const MatrixWorkspace_const_sptr outWS = runAlgorithm(alg, inWS);

    // specific checks
    const MantidVec &outX = outWS->readX(0);
    TS_ASSERT_EQUALS(outX.size(), 1);
    TS_ASSERT_DELTA(outX[0], 0.0, 1e-6);

    const MantidVec &outY0 = outWS->readY(0);
    TS_ASSERT_EQUALS(outY0.size(), 1);
    TS_ASSERT_DELTA(outY0[0], 1.0, 1e-6);

    const MantidVec &outY1 = outWS->readY(1);
    TS_ASSERT_EQUALS(outY1.size(), 1);
    TS_ASSERT_DELTA(outY1[0], -1.0, 1e-6);
  }

  void testMaxDSpaceShift() {
    CrossCorrelate alg;
    // TODO need a duplicate test for other shape
    auto inputWS = makeFakeWorkspace3Peaks(PeakShapeEnum::GAUSSIAN);
    TS_ASSERT(inputWS != nullptr);

    const MatrixWorkspace_const_sptr inWS = setupAlgorithm(alg, inputWS, 0.0, 4.0, 5);
    const MatrixWorkspace_const_sptr outWS = runAlgorithm(alg, inWS);

    // TODO add validation
  }

  void testInputXLength2() {
    // this throws because at least 3 X values are required
    CrossCorrelate alg;
    setupAlgorithm(alg, 2.0, 3.0);
    runAlgorithmThrows(alg);
  }

  void testInputXLength1() {
    // this throws because at least 3 X values are required
    CrossCorrelate alg;
    setupAlgorithm(alg, 2.0, 2.4);
    runAlgorithmThrows(alg);
  }

  void testXMinEqualsXMax() {
    // this throws because XMin should be > XMax
    CrossCorrelate alg;
    setupAlgorithm(alg, 2.0, 2.0);
    runAlgorithmThrows(alg);
  }

  void testXMinGreaterThanXMax() {
    // this throws because XMin should be < XMax
    CrossCorrelate alg;
    setupAlgorithm(alg, 3.0, 2.0);
    runAlgorithmThrows(alg);
  }

private:
  const MatrixWorkspace_sptr makeFakeWorkspace() {
    // create the x and e values
    const int nBins = 10;
    BinEdges xValues(nBins + 1, HistogramData::LinearGenerator(0.0, 0.5));
    CountStandardDeviations e1(nBins, sqrt(3.0));

    // for y use a gaussian peak centred at 2.5 with height=10
    const double sigmaSq = 0.7 * 0.7;
    std::vector<double> yValues(nBins);
    for (size_t j = 0; j < nBins; ++j) {
      yValues[j] = 0.3 + 10.0 * exp(-0.5 * pow(xValues[j] - 2.5, 2.0) / sigmaSq);
    }

    // create the workspace
    const int nHist = 2;
    const MatrixWorkspace_sptr ws = createWorkspace<Workspace2D>(nHist, nBins + 1, nBins);
    ws->getAxis(0)->setUnit("dSpacing");

    for (size_t i = 0; i < nHist; ++i) {
      ws->setBinEdges(i, xValues);
      ws->mutableY(i) = yValues;
      ws->setCountStandardDeviations(i, e1);

      // offset the x values for the next spectrum
      xValues += 0.5;
    }

    return ws;
  }

  const MatrixWorkspace_sptr makeFakeWorkspace3Peaks(const PeakShapeEnum shape) {
    const double D_MIN{0.9};
    const double D_MAX{2.3};
    const double D_DELTA{0.01};
    const int NUM_BINS = static_cast<int>((D_MAX - D_MIN) / D_DELTA);
    const int NUM_HIST{5};

    // create the x-axis
    BinEdges xEdges(NUM_BINS + 1, HistogramData::LinearGenerator(D_MIN, D_DELTA));
    std::vector<double> xValues(cbegin(xEdges), cend(xEdges) - 1);
    // create the uncertainties
    CountStandardDeviations e1(NUM_BINS, sqrt(3.0)); // arbitrary

    // create workspace to put everything into
    MatrixWorkspace_sptr ws = createWorkspace<Workspace2D>(NUM_HIST, NUM_BINS + 1, NUM_BINS);
    ws->getAxis(0)->setUnit("dSpacing");

    // loop over the spectra
    // which spectra is which is coded in createcompositeB2BExp
    for (int spectrumIndex = 0; spectrumIndex < NUM_HIST; ++spectrumIndex) {
      auto compositefunction = createCompositeB2BExp(shape, spectrumIndex);
      ws->setBinEdges(spectrumIndex, xEdges);
      ws->setCounts(spectrumIndex, evaluateFunction(compositefunction, xValues));
    }

    return ws;
  }

  void setupAlgorithmPropsBasic(CrossCorrelate &alg, const double xmin, const double xmax) {
    // set up the algorithm
    if (!alg.isInitialized())
      alg.initialize();
    alg.setChild(true);
    alg.setProperty("OutputWorkspace", "outWS");
    alg.setProperty("ReferenceSpectra", 0);
    alg.setProperty("WorkspaceIndexMin", 0);
    alg.setProperty("WorkspaceIndexMax", 1);
    alg.setProperty("XMin", xmin);
    alg.setProperty("XMax", xmax);
  }
  // Initialise the algorithm and set the properties. Creates a fake workspace
  // for the input and returns it.
  MatrixWorkspace_const_sptr setupAlgorithm(CrossCorrelate &alg, const double xmin, const double xmax) {

    // create the workspace
    const MatrixWorkspace_sptr inWS = makeFakeWorkspace();

    setupAlgorithmPropsBasic(alg, xmin, xmax);
    alg.setProperty("InputWorkspace", inWS);

    return inWS;
  }

  // Initialise the algorithm and set the properties. Creates a fake workspace
  // for the input and returns it.
  MatrixWorkspace_const_sptr setupAlgorithm(CrossCorrelate &alg, const MatrixWorkspace_sptr inWS, const double xmin,
                                            const double xmax, const double maxDSpaceShift) {

    // create the workspace
    setupAlgorithmPropsBasic(alg, xmin, xmax);
    alg.setProperty("InputWorkspace", inWS);
    alg.setProperty("MaxDspaceShift", maxDSpaceShift);

    return inWS;
  }

  // Run the algorithm and do some basic checks. Returns the output workspace.
  MatrixWorkspace_const_sptr runAlgorithm(CrossCorrelate &alg, const MatrixWorkspace_const_sptr &inWS) {
    // run the algorithm
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    // verify the output workspace
    const MatrixWorkspace_const_sptr outWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT_EQUALS(inWS->getNumberHistograms(),
                     outWS->getNumberHistograms()); // shouldn't drop histograms

    return outWS;
  }

  // Run the algorithm with invalid input and check that it throws a runtime
  // error
  void runAlgorithmThrows(CrossCorrelate &alg) { TS_ASSERT_THROWS(alg.execute(), const std::runtime_error &); }
};
