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
#include "MantidAlgorithms/GroupWorkspaces.h"
#include "MantidAlgorithms/Stitch.h"
#include "MantidHistogramData/Histogram.h"

#include <math.h>

using namespace Mantid::Algorithms;
using namespace Mantid::API;
using namespace Mantid::HistogramData;

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

  void test_WorkspaceGroup() {
    auto ws1 = pointDataWorkspaceOneSpectrum(12, 0.3, 0.7, "ws1");
    auto ws2 = pointDataWorkspaceOneSpectrum(17, 0.5, 0.9, "ws2");
    GroupWorkspaces grouper;
    grouper.initialize();
    grouper.setAlwaysStoreInADS(true);
    grouper.setProperty("InputWorkspaces", std::vector<std::string>({"ws1", "ws2"}));
    grouper.setPropertyValue("OutputWorkspace", "group");
    grouper.execute();

    Stitch alg;
    alg.setRethrows(true);
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspaces", "group"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OutputWorkspace", "out"));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
  }

  void test_IncompatibleWorkspaces() {
    auto ws1 = pointDataWorkspaceOneSpectrum(12, 0.3, 0.7, "ws1");
    auto ws2 = histoDataWorkspaceOneSpectrum(11, 0.5, 0.9, "ws2");
    Stitch alg;
    alg.setRethrows(true);
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspaces", std::vector<std::string>({"ws1", "ws2"})));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OutputWorkspace", "out"))
    TS_ASSERT_THROWS_EQUALS(alg.execute(), const std::runtime_error &e, std::string(e.what()),
                            "Some invalid Properties found: [ InputWorkspaces ]");
  }

  void test_WorkspacesAndGroupsMixed() {}

  void test_NotEnoughOverlap() {}

  void test_NoExplicitReference() {}

  void test_ExplicitReference() {}

  void test_PointData() {}

  void test_HistogramData() {}

  void test_MultipleSpectra() {}

  void test_LeftToRight() {}

  void test_RightToLeft() {}

  void test_CustomOrder() {}

  void test_MultiSpectra() {}

  void test_Ragged() {}

  void test_ManualScaleFactors() {}

  void test_NoScaling() {}

  void test_TiedScaleFactor() {}

private:
  MatrixWorkspace_sptr pointDataWorkspaceOneSpectrum(size_t nPoints, double startX, double endX,
                                                     const std::string &name) {
    MatrixWorkspace_sptr ws = WorkspaceFactory::Instance().create("Workspace2D", 1, nPoints, nPoints);
    AnalysisDataService::Instance().addOrReplace(name, ws);
    std::vector<double> x(nPoints), y(nPoints), e(nPoints);
    const double step = (endX - startX) / (nPoints - 1);
    for (size_t ibin = 0; ibin < nPoints; ++ibin) {
      x[ibin] = startX + ibin * step;
      y[ibin] = 7 * ibin + 3;
      e[ibin] = std::sqrt(y[ibin]);
    }
    ws->setHistogram(0, Histogram(Points(x), Counts(y), CountStandardDeviations(e)));
    return ws;
  }

  MatrixWorkspace_sptr histoDataWorkspaceOneSpectrum(size_t nBins, double startX, double endX,
                                                     const std::string &name) {
    MatrixWorkspace_sptr ws = WorkspaceFactory::Instance().create("Workspace2D", 1, nBins + 1, nBins);
    AnalysisDataService::Instance().addOrReplace(name, ws);
    std::vector<double> x(nBins + 1), y(nBins), e(nBins);
    const double step = (endX - startX) / nBins;
    for (size_t ibin = 0; ibin < nBins; ++ibin) {
      x[ibin] = startX + ibin * step;
      y[ibin] = 7 * ibin + 3;
      e[ibin] = std::sqrt(y[ibin]);
    }
    x[nBins] = endX;
    ws->setHistogram(0, Histogram(BinEdges(x), Counts(y), CountStandardDeviations(e)));
    return ws;
  }
};
