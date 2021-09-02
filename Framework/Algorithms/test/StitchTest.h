// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceGroup_fwd.h"
#include "MantidAlgorithms/CompareWorkspaces.h"
#include "MantidAlgorithms/ConjoinXRuns.h"
#include "MantidAlgorithms/CropWorkspace.h"
#include "MantidAlgorithms/GroupWorkspaces.h"
#include "MantidAlgorithms/Multiply.h"
#include "MantidAlgorithms/SortXAxis.h"
#include "MantidAlgorithms/Stitch.h"
#include "MantidHistogramData/Histogram.h"

#include <math.h>

using namespace Mantid::Algorithms;
using namespace Mantid::API;
using namespace Mantid::HistogramData;

namespace {

MatrixWorkspace_sptr pointDataWorkspaceOneSpectrum(size_t nPoints, double startX, double endX,
                                                   const std::string &name) {
  MatrixWorkspace_sptr ws = WorkspaceFactory::Instance().create("Workspace2D", 1, nPoints, nPoints);
  AnalysisDataService::Instance().addOrReplace(name, ws);
  std::vector<double> x(nPoints), y(nPoints), e(nPoints);
  const double step = (endX - startX) / (double(nPoints) - 1);
  for (size_t ibin = 0; ibin < nPoints; ++ibin) {
    x[ibin] = startX + double(ibin) * step;
    y[ibin] = 7 * double(ibin) + 3;
    e[ibin] = std::sqrt(y[ibin]);
  }
  ws->setHistogram(0, Histogram(Points(x), Counts(y), CountStandardDeviations(e)));
  return ws;
}

MatrixWorkspace_sptr histoDataWorkspaceOneSpectrum(size_t nPoints, double startX, double endX,
                                                   const std::string &name) {
  MatrixWorkspace_sptr ws = WorkspaceFactory::Instance().create("Workspace2D", 1, nPoints, nPoints);
  AnalysisDataService::Instance().addOrReplace(name, ws);
  std::vector<double> x(nPoints + 1), y(nPoints), e(nPoints);
  const double step = (endX - startX) / double(nPoints);
  for (size_t ibin = 0; ibin < nPoints; ++ibin) {
    x[ibin] = startX + double(ibin) * step;
    y[ibin] = 7 * double(ibin) + 3;
    e[ibin] = std::sqrt(y[ibin]);
  }
  x[nPoints] = endX;
  ws->setHistogram(0, Histogram(BinEdges(x), Counts(y), CountStandardDeviations(e)));
  return ws;
}

MatrixWorkspace_sptr pointDataWorkspaceMultiSpectrum(size_t nSpectra, size_t nPoints, double startX, double endX,
                                                     const std::string &name) {
  MatrixWorkspace_sptr ws = WorkspaceFactory::Instance().create("Workspace2D", nSpectra, nPoints, nPoints);
  AnalysisDataService::Instance().addOrReplace(name, ws);
  std::vector<double> x(nPoints), y(nPoints), e(nPoints);
  const double step = (endX - startX) / (double(nPoints) - 1);
  for (size_t ispec = 0; ispec < nSpectra; ++ispec) {
    for (size_t ibin = 0; ibin < nPoints; ++ibin) {
      x[ibin] = startX + double(ibin) * step;
      y[ibin] = 7 * double(ibin) + 3 + 10 * double(ispec);
      e[ibin] = std::sqrt(y[ibin]);
    }
    ws->setHistogram(ispec, Histogram(Points(x), Counts(y), CountStandardDeviations(e)));
  }
  return ws;
}

MatrixWorkspace_sptr pointDataWorkspaceMultiSpectrumRagged(size_t nSpectra, size_t nPoints, double startX, double endX,
                                                           const std::string &name) {
  MatrixWorkspace_sptr ws = WorkspaceFactory::Instance().create("Workspace2D", nSpectra, nPoints, nPoints);
  AnalysisDataService::Instance().addOrReplace(name, ws);
  std::vector<double> x(nPoints), y(nPoints), e(nPoints);
  const double step = (endX - startX) / (double(nPoints) - 1);
  for (size_t ispec = 0; ispec < nSpectra; ++ispec) {
    for (size_t ibin = 0; ibin < nPoints; ++ibin) {
      x[ibin] = startX + double(ibin) * step + 0.01 * double(ispec);
      y[ibin] = 7 * double(ibin) + 3 + 10 * double(ispec);
      e[ibin] = std::sqrt(y[ibin]);
    }
    ws->setHistogram(ispec, Histogram(Points(x), Counts(y), CountStandardDeviations(e)));
  }
  return ws;
}
} // namespace

class StitchTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static StitchTest *createSuite() { return new StitchTest(); }
  static void destroySuite(StitchTest *suite) { delete suite; }

  void tearDown() override { AnalysisDataService::Instance().clear(); }

  void test_Init() {
    Stitch alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  //================================FAILURE CASES===================================//
  void test_NoOverlap() {
    auto ws1 = pointDataWorkspaceOneSpectrum(12, 0.3, 0.7, "ws1");
    auto ws2 = pointDataWorkspaceOneSpectrum(17, 0.8, 0.9, "ws2");
    Stitch alg;
    alg.setRethrows(true);
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspaces", std::vector<std::string>({"ws1", "ws2"})));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OutputWorkspace", "out"))
    TS_ASSERT_THROWS_EQUALS(alg.execute(), const std::runtime_error &e, std::string(e.what()),
                            "No overlap is found between the intervals: [0.3,0.7] and [0.8, 0.9]");
  }

  void test_Ragged() {
    auto ws1 = pointDataWorkspaceMultiSpectrumRagged(3, 17, 0.5, 0.9, "ws1");
    auto ws2 = pointDataWorkspaceMultiSpectrumRagged(3, 13, 0.8, 1.1, "ws2");
    Stitch alg;
    alg.setRethrows(true);
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspaces", std::vector<std::string>({"ws1", "ws2"})));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OutputWorkspace", "out"))
    TS_ASSERT_THROWS_EQUALS(alg.execute(), const std::runtime_error &e, std::string(e.what()),
                            "Some invalid Properties found: [ InputWorkspaces ]");
  }

  void test_OneWorkspace() {
    auto ws1 = pointDataWorkspaceOneSpectrum(12, 0.3, 0.7, "ws1");
    Stitch alg;
    alg.setRethrows(true);
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspaces", std::vector<std::string>({"ws1"})));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OutputWorkspace", "out"))
    TS_ASSERT_THROWS_EQUALS(alg.execute(), const std::runtime_error &e, std::string(e.what()),
                            "Some invalid Properties found: [ InputWorkspaces ]");
  }

  void test_HistogramData() {
    auto ws1 = histoDataWorkspaceOneSpectrum(12, 0.3, 0.7, "ws1");
    auto ws2 = histoDataWorkspaceOneSpectrum(17, 0.8, 0.9, "ws2");
    Stitch alg;
    alg.setRethrows(true);
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspaces", std::vector<std::string>({"ws1", "ws2"})));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OutputWorkspace", "out"))
    TS_ASSERT_THROWS_EQUALS(alg.execute(), const std::runtime_error &e, std::string(e.what()),
                            "Some invalid Properties found: [ InputWorkspaces ]");
  }

  void test_IncompatibleWorkspaces() {
    auto ws1 = pointDataWorkspaceOneSpectrum(12, 0.3, 0.7, "ws1");
    auto ws2 = pointDataWorkspaceMultiSpectrum(3, 11, 0.5, 0.9, "ws2");
    Stitch alg;
    alg.setRethrows(true);
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspaces", std::vector<std::string>({"ws1", "ws2"})));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OutputWorkspace", "out"))
    TS_ASSERT_THROWS_EQUALS(alg.execute(), const std::runtime_error &e, std::string(e.what()),
                            "Some invalid Properties found: [ InputWorkspaces ]");
  }

  void test_NotEnoughOverlap() {
    auto ws1 = pointDataWorkspaceOneSpectrum(5, 0.1, 0.6, "ws1");
    auto ws2 = pointDataWorkspaceOneSpectrum(7, 0.5, 1.2, "ws2");
    Stitch alg;
    alg.setRethrows(true);
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspaces", std::vector<std::string>({"ws1", "ws2"})));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OutputWorkspace", "out"))
    TS_ASSERT_THROWS_EQUALS(alg.execute(), const std::runtime_error &e, std::string(e.what()),
                            "Unable to make the ratio; only one overlapping point is found and it is at different x");
  }

  //================================HAPPY CASES===================================//
  void test_WorkspaceGroup() {
    auto ws1 = pointDataWorkspaceOneSpectrum(11, 0.3, 0.7, "ws1");
    auto ws2 = pointDataWorkspaceOneSpectrum(21, 0.55, 0.95, "ws2");
    const std::vector<std::string> inputs({"ws1", "ws2"});
    GroupWorkspaces grouper;
    grouper.initialize();
    grouper.setAlwaysStoreInADS(true);
    grouper.setProperty("InputWorkspaces", inputs);
    grouper.setPropertyValue("OutputWorkspace", "group");
    grouper.execute();

    Stitch alg;
    alg.setRethrows(true);
    alg.setChild(true);
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InputWorkspaces", "group"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "out"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputScaleFactorsWorkspace", "factors"));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());
    MatrixWorkspace_sptr stitched = alg.getProperty("OutputWorkspace");
    MatrixWorkspace_sptr factors = alg.getProperty("OutputScaleFactorsWorkspace");
    TS_ASSERT(crossCheckStitch(inputs, stitched, factors));
    TS_ASSERT_EQUALS(factors->readY(0)[0], 1.)
    TS_ASSERT_DIFFERS(factors->readY(0)[1], 1.)
  }

  void test_WorkspacesAndGroupsMixed() {
    auto ws1 = pointDataWorkspaceOneSpectrum(12, 0.3, 0.7, "ws1");
    auto ws2 = pointDataWorkspaceOneSpectrum(17, 0.5, 0.9, "ws2");
    auto ws3 = pointDataWorkspaceOneSpectrum(19, 0.8, 1.1, "ws3");
    GroupWorkspaces grouper;
    grouper.initialize();
    grouper.setAlwaysStoreInADS(true);
    grouper.setProperty("InputWorkspaces", std::vector<std::string>({"ws1", "ws2"}));
    grouper.setPropertyValue("OutputWorkspace", "group");
    grouper.execute();

    Stitch alg;
    alg.setRethrows(true);
    alg.setChild(true);
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspaces", std::vector<std::string>({"group", "ws3"})));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "out"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputScaleFactorsWorkspace", "factors"));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());
    MatrixWorkspace_sptr stitched = alg.getProperty("OutputWorkspace");
    MatrixWorkspace_sptr factors = alg.getProperty("OutputScaleFactorsWorkspace");
    TS_ASSERT(crossCheckStitch(std::vector<std::string>({"ws1", "ws2", "ws3"}), stitched, factors));
    TS_ASSERT_EQUALS(factors->readY(0)[0], 1.)
    TS_ASSERT_DIFFERS(factors->readY(0)[1], 1.)
    TS_ASSERT_DIFFERS(factors->readY(0)[1], 1.)
  }

  void test_NoExplicitReference() {
    auto ws1 = pointDataWorkspaceOneSpectrum(12, 0.3, 0.7, "ws1");
    auto ws2 = pointDataWorkspaceOneSpectrum(17, 0.5, 0.9, "ws2");
    Stitch alg;
    alg.setRethrows(true);
    alg.setChild(true);
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspaces", std::vector<std::string>({"ws1", "ws2"})));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "out"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputScaleFactorsWorkspace", "factors"));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());
    MatrixWorkspace_sptr stitched = alg.getProperty("OutputWorkspace");
    MatrixWorkspace_sptr factors = alg.getProperty("OutputScaleFactorsWorkspace");
    TS_ASSERT(crossCheckStitch(std::vector<std::string>({"ws1", "ws2"}), stitched, factors));
    TS_ASSERT_EQUALS(factors->readY(0)[0], 1.)
    TS_ASSERT_DIFFERS(factors->readY(0)[1], 1.)
  }

  void test_ExplicitReference() {
    auto ws1 = pointDataWorkspaceOneSpectrum(12, 0.3, 0.7, "ws1");
    auto ws2 = pointDataWorkspaceOneSpectrum(17, 0.5, 0.9, "ws2");
    Stitch alg;
    alg.setRethrows(true);
    alg.setChild(true);
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspaces", std::vector<std::string>({"ws1", "ws2"})));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("ReferenceWorkspace", "ws2"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "out"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputScaleFactorsWorkspace", "factors"));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());
    MatrixWorkspace_sptr stitched = alg.getProperty("OutputWorkspace");
    MatrixWorkspace_sptr factors = alg.getProperty("OutputScaleFactorsWorkspace");
    TS_ASSERT(crossCheckStitch(std::vector<std::string>({"ws1", "ws2"}), stitched, factors));
    TS_ASSERT_DIFFERS(factors->readY(0)[0], 1.)
    TS_ASSERT_EQUALS(factors->readY(0)[1], 1.)
  }

  void test_LeftToRight() {
    auto ws1 = pointDataWorkspaceOneSpectrum(12, 0.3, 0.7, "ws1");
    auto ws2 = pointDataWorkspaceOneSpectrum(17, 0.5, 0.9, "ws2");
    auto ws3 = pointDataWorkspaceOneSpectrum(19, 0.8, 1.3, "ws3");
    Stitch alg;
    alg.setRethrows(true);
    alg.setChild(true);
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspaces", std::vector<std::string>({"ws1", "ws2", "ws3"})));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "out"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputScaleFactorsWorkspace", "factors"));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());
    MatrixWorkspace_sptr stitched = alg.getProperty("OutputWorkspace");
    MatrixWorkspace_sptr factors = alg.getProperty("OutputScaleFactorsWorkspace");
    TS_ASSERT(crossCheckStitch(std::vector<std::string>({"ws1", "ws2", "ws3"}), stitched, factors));
    TS_ASSERT_EQUALS(factors->readY(0)[0], 1.)
    TS_ASSERT_DIFFERS(factors->readY(0)[1], 1.)
    TS_ASSERT_DIFFERS(factors->readY(0)[2], 1.)
  }

  void test_RightToLeft() {
    auto ws1 = pointDataWorkspaceOneSpectrum(12, 0.3, 0.7, "ws1");
    auto ws2 = pointDataWorkspaceOneSpectrum(17, 0.5, 0.9, "ws2");
    auto ws3 = pointDataWorkspaceOneSpectrum(19, 0.8, 1.3, "ws3");
    Stitch alg;
    alg.setRethrows(true);
    alg.setChild(true);
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspaces", std::vector<std::string>({"ws3", "ws2", "ws1"})));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "out"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputScaleFactorsWorkspace", "factors"));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());
    MatrixWorkspace_sptr stitched = alg.getProperty("OutputWorkspace");
    MatrixWorkspace_sptr factors = alg.getProperty("OutputScaleFactorsWorkspace");
    TS_ASSERT(crossCheckStitch(std::vector<std::string>({"ws3", "ws2", "ws1"}), stitched, factors));
    TS_ASSERT_DIFFERS(factors->readY(0)[0], 1.)
    TS_ASSERT_DIFFERS(factors->readY(0)[1], 1.)
    TS_ASSERT_EQUALS(factors->readY(0)[2], 1.)
  }

  void test_CustomOrder() {
    auto ws1 = pointDataWorkspaceOneSpectrum(12, 0.3, 0.7, "ws1");
    auto ws2 = pointDataWorkspaceOneSpectrum(17, 0.5, 0.9, "ws2");
    auto ws3 = pointDataWorkspaceOneSpectrum(19, 0.8, 1.3, "ws3");
    Stitch alg;
    alg.setRethrows(true);
    alg.setChild(true);
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspaces", std::vector<std::string>({"ws3", "ws1", "ws2"})));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "out"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputScaleFactorsWorkspace", "factors"));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());
    MatrixWorkspace_sptr stitched = alg.getProperty("OutputWorkspace");
    MatrixWorkspace_sptr factors = alg.getProperty("OutputScaleFactorsWorkspace");
    TS_ASSERT(crossCheckStitch(std::vector<std::string>({"ws3", "ws1", "ws2"}), stitched, factors));
    TS_ASSERT_DIFFERS(factors->readY(0)[0], 1.)
    TS_ASSERT_EQUALS(factors->readY(0)[1], 1.)
    TS_ASSERT_DIFFERS(factors->readY(0)[2], 1.)
  }

  void test_ManualScaleFactors() {
    auto ws1 = pointDataWorkspaceOneSpectrum(12, 0.3, 0.7, "ws1");
    auto ws2 = pointDataWorkspaceOneSpectrum(17, 0.5, 0.9, "ws2");
    auto ws3 = pointDataWorkspaceOneSpectrum(19, 0.8, 1.3, "ws3");
    Stitch alg;
    alg.setRethrows(true);
    alg.setChild(true);
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspaces", std::vector<std::string>({"ws3", "ws1", "ws2"})));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("ScaleFactorCalculation", "Manual"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("ManualScaleFactors", std::vector<double>({9.1, 31.7, 11.19})));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "out"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputScaleFactorsWorkspace", "factors"));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());
    MatrixWorkspace_sptr stitched = alg.getProperty("OutputWorkspace");
    MatrixWorkspace_sptr factors = alg.getProperty("OutputScaleFactorsWorkspace");
    TS_ASSERT_EQUALS(factors->getNumberHistograms(), 1)
    TS_ASSERT(crossCheckStitch(std::vector<std::string>({"ws3", "ws1", "ws2"}), stitched, factors));
    TS_ASSERT_EQUALS(factors->readY(0)[0], 9.1)
    TS_ASSERT_EQUALS(factors->readY(0)[1], 31.7)
    TS_ASSERT_EQUALS(factors->readY(0)[2], 11.19)
  }

  void test_NoScaling() {
    auto ws1 = pointDataWorkspaceOneSpectrum(12, 0.3, 0.7, "ws1");
    auto ws2 = pointDataWorkspaceOneSpectrum(17, 0.5, 0.9, "ws2");
    auto ws3 = pointDataWorkspaceOneSpectrum(19, 0.8, 1.3, "ws3");
    Stitch alg;
    alg.setRethrows(true);
    alg.setChild(true);
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspaces", std::vector<std::string>({"ws3", "ws1", "ws2"})));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("ScaleFactorCalculation", "Manual"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("ManualScaleFactors", std::vector<double>({1., 1., 1.})));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "out"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputScaleFactorsWorkspace", "factors"));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());
    MatrixWorkspace_sptr stitched = alg.getProperty("OutputWorkspace");
    MatrixWorkspace_sptr factors = alg.getProperty("OutputScaleFactorsWorkspace");
    TS_ASSERT_EQUALS(factors->getNumberHistograms(), 1)
    TS_ASSERT_EQUALS(factors->readY(0)[0], 1.)
    TS_ASSERT_EQUALS(factors->readY(0)[1], 1.)
    TS_ASSERT_EQUALS(factors->readY(0)[2], 1.)
    TS_ASSERT(crossCheckStitch(std::vector<std::string>({"ws3", "ws1", "ws2"}), stitched, factors));
  }

  void test_MultiSpectra() {
    auto ws1 = pointDataWorkspaceMultiSpectrum(3, 12, 0.3, 0.7, "ws1");
    auto ws2 = pointDataWorkspaceMultiSpectrum(3, 17, 0.5, 0.9, "ws2");
    auto ws3 = pointDataWorkspaceMultiSpectrum(3, 19, 0.8, 1.3, "ws3");
    Stitch alg;
    alg.setRethrows(true);
    alg.setChild(true);
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspaces", std::vector<std::string>({"ws1", "ws2", "ws3"})));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "out"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputScaleFactorsWorkspace", "factors"));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());
    MatrixWorkspace_sptr stitched = alg.getProperty("OutputWorkspace");
    MatrixWorkspace_sptr factors = alg.getProperty("OutputScaleFactorsWorkspace");
    TS_ASSERT_EQUALS(factors->readY(0)[0], 1.)
    TS_ASSERT_DIFFERS(factors->readY(0)[1], 1.)
    TS_ASSERT_DIFFERS(factors->readY(0)[2], 1.)
    TS_ASSERT(crossCheckStitch(std::vector<std::string>({"ws1", "ws2", "ws3"}), stitched, factors));
  }

  void test_TiedScaleFactor() {
    auto ws1 = pointDataWorkspaceMultiSpectrum(3, 12, 0.3, 0.7, "ws1");
    auto ws2 = pointDataWorkspaceMultiSpectrum(3, 17, 0.5, 0.9, "ws2");
    auto ws3 = pointDataWorkspaceMultiSpectrum(3, 19, 0.8, 1.3, "ws3");
    Stitch alg;
    alg.setRethrows(true);
    alg.setChild(true);
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspaces", std::vector<std::string>({"ws1", "ws2", "ws3"})));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("TieScaleFactors", true));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "out"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputScaleFactorsWorkspace", "factors"));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());
    MatrixWorkspace_sptr stitched = alg.getProperty("OutputWorkspace");
    MatrixWorkspace_sptr factors = alg.getProperty("OutputScaleFactorsWorkspace");
    TS_ASSERT_EQUALS(factors->getNumberHistograms(), 1)
    TS_ASSERT(crossCheckStitch(std::vector<std::string>({"ws1", "ws2", "ws3"}), stitched, factors));
    TS_ASSERT_EQUALS(factors->readY(0)[0], 1.)
    TS_ASSERT_DIFFERS(factors->readY(0)[1], 1.)
    TS_ASSERT_DIFFERS(factors->readY(0)[2], 1.)
  }

private:
  bool crossCheckStitch(const std::vector<std::string> &inputs, MatrixWorkspace_sptr stitched,
                        MatrixWorkspace_sptr factors) {
    MatrixWorkspace_sptr expected = expectedStitchedOutput(inputs, factors);
    CompareWorkspaces comparator;
    comparator.initialize();
    comparator.setChild(true);
    comparator.setProperty("Workspace1", stitched);
    comparator.setProperty("Workspace2", expected);
    comparator.execute();
    return comparator.getProperty("Result");
  }

  MatrixWorkspace_sptr expectedStitchedOutput(const std::vector<std::string> &inputs, MatrixWorkspace_sptr factors) {
    for (size_t ws = 0; ws < inputs.size(); ++ws) {
      CropWorkspace cropper;
      cropper.setChild(true);
      cropper.initialize();
      cropper.setProperty("InputWorkspace", factors);
      cropper.setProperty("XMin", double(ws) + 0.5);
      cropper.setProperty("XMax", double(ws) + 1.5);
      cropper.setPropertyValue("OutputWorkspace", "__tmp");
      cropper.execute();
      MatrixWorkspace_sptr factorsColumn = cropper.getProperty("OutputWorkspace");
      Multiply multiplier;
      multiplier.initialize();
      multiplier.setChild(true);
      multiplier.setPropertyValue("LHSWorkspace", inputs[ws]);
      multiplier.setProperty("RHSWorkspace", factorsColumn);
      multiplier.setPropertyValue("OutputWorkspace", inputs[ws]);
      multiplier.execute();
    }
    ConjoinXRuns conjoiner;
    conjoiner.initialize();
    conjoiner.setChild(true);
    conjoiner.setProperty("InputWorkspaces", inputs);
    conjoiner.setPropertyValue("OutputWorkspace", "__joined");
    conjoiner.execute();
    Workspace_sptr joined = conjoiner.getProperty("OutputWorkspace");
    SortXAxis sorter;
    sorter.initialize();
    sorter.setChild(true);
    sorter.setProperty("InputWorkspace", joined);
    sorter.setPropertyValue("OutputWorkspace", "__sorted");
    sorter.execute();
    MatrixWorkspace_sptr sorted = sorter.getProperty("OutputWorkspace");
    return sorted;
  }
};

class StitchTestPerformance : public CxxTest::TestSuite {
public:
  static StitchTestPerformance *createSuite() { return new StitchTestPerformance(); }
  static void destroySuite(StitchTestPerformance *suite) { delete suite; }

  StitchTestPerformance() {}

  void setUp() override {
    m_alg.initialize();
    std::vector<std::string> inputs;
    inputs.reserve(100);
    for (size_t i = 0; i < 50; ++i) {
      inputs.emplace_back("ws" + std::to_string(i));
      pointDataWorkspaceMultiSpectrum(1000, 99, 7 + double(i), 9 + double(i), inputs[i]);
    }
    m_alg.setProperty("InputWorkspaces", inputs);
    m_alg.setPropertyValue("OutputWorkspace", "__out_ws");
  }

  void tearDown() override { AnalysisDataService::Instance().clear(); }

  void test_performance() { TS_ASSERT_THROWS_NOTHING(m_alg.execute()); }

private:
  Stitch m_alg;
};
