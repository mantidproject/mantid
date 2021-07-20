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
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OutputScaleFactorsWorkspace", "factors"));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
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
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspaces", std::vector<std::string>({"group", "ws3"})));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OutputWorkspace", "out"));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
  }

  void test_NoExplicitReference() {
    auto ws1 = pointDataWorkspaceOneSpectrum(12, 0.3, 0.7, "ws1");
    auto ws2 = pointDataWorkspaceOneSpectrum(17, 0.5, 0.9, "ws2");
    Stitch alg;
    alg.setRethrows(true);
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspaces", std::vector<std::string>({"ws1", "ws2"})));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OutputWorkspace", "out"));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
  }

  void test_ExplicitReference() {
    auto ws1 = pointDataWorkspaceOneSpectrum(12, 0.3, 0.7, "ws1");
    auto ws2 = pointDataWorkspaceOneSpectrum(17, 0.5, 0.9, "ws2");
    Stitch alg;
    alg.setRethrows(true);
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspaces", std::vector<std::string>({"ws1", "ws2"})));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("ReferenceWorkspace", "ws2"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OutputWorkspace", "out"));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
  }

  void test_LeftToRight() {
    auto ws1 = pointDataWorkspaceOneSpectrum(12, 0.3, 0.7, "ws1");
    auto ws2 = pointDataWorkspaceOneSpectrum(17, 0.5, 0.9, "ws2");
    auto ws3 = pointDataWorkspaceOneSpectrum(19, 0.8, 1.3, "ws3");
    Stitch alg;
    alg.setRethrows(true);
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspaces", std::vector<std::string>({"ws1", "ws2", "ws3"})));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OutputWorkspace", "out"));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
  }

  void test_RightToLeft() {
    auto ws1 = pointDataWorkspaceOneSpectrum(12, 0.3, 0.7, "ws1");
    auto ws2 = pointDataWorkspaceOneSpectrum(17, 0.5, 0.9, "ws2");
    auto ws3 = pointDataWorkspaceOneSpectrum(19, 0.8, 1.3, "ws3");
    Stitch alg;
    alg.setRethrows(true);
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspaces", std::vector<std::string>({"ws3", "ws2", "ws1"})));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OutputWorkspace", "out"));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
  }

  void test_CustomOrder() {
    auto ws1 = pointDataWorkspaceOneSpectrum(12, 0.3, 0.7, "ws1");
    auto ws2 = pointDataWorkspaceOneSpectrum(17, 0.5, 0.9, "ws2");
    auto ws3 = pointDataWorkspaceOneSpectrum(19, 0.8, 1.3, "ws3");
    Stitch alg;
    alg.setRethrows(true);
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspaces", std::vector<std::string>({"ws3", "ws1", "ws2"})));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OutputWorkspace", "out"));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
  }

  void test_ManualScaleFactors() {
    auto ws1 = pointDataWorkspaceOneSpectrum(12, 0.3, 0.7, "ws1");
    auto ws2 = pointDataWorkspaceOneSpectrum(17, 0.5, 0.9, "ws2");
    auto ws3 = pointDataWorkspaceOneSpectrum(19, 0.8, 1.3, "ws3");
    Stitch alg;
    alg.setRethrows(true);
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspaces", std::vector<std::string>({"ws3", "ws1", "ws2"})));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("ScaleFactorCalculation", "Manual"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("ManualScaleFactors", std::vector<double>({9.1, 31.7, 11.19})));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OutputWorkspace", "out"));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
  }

  void test_NoScaling() {
    auto ws1 = pointDataWorkspaceOneSpectrum(12, 0.3, 0.7, "ws1");
    auto ws2 = pointDataWorkspaceOneSpectrum(17, 0.5, 0.9, "ws2");
    auto ws3 = pointDataWorkspaceOneSpectrum(19, 0.8, 1.3, "ws3");
    Stitch alg;
    alg.setRethrows(true);
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspaces", std::vector<std::string>({"ws3", "ws1", "ws2"})));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("ScaleFactorCalculation", "Manual"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("ManualScaleFactors", std::vector<double>({1., 1., 1.})));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OutputWorkspace", "out"));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
  }

  void test_MultiSpectra() {
    auto ws1 = pointDataWorkspaceMultiSpectrum(3, 12, 0.3, 0.7, "ws1");
    auto ws2 = pointDataWorkspaceMultiSpectrum(3, 17, 0.5, 0.9, "ws2");
    auto ws3 = pointDataWorkspaceMultiSpectrum(3, 19, 0.8, 1.3, "ws3");
    Stitch alg;
    alg.setRethrows(true);
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspaces", std::vector<std::string>({"ws1", "ws2", "ws3"})));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OutputWorkspace", "out"));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
  }

  void test_TiedScaleFactor() {
    auto ws1 = pointDataWorkspaceMultiSpectrum(3, 12, 0.3, 0.7, "ws1");
    auto ws2 = pointDataWorkspaceMultiSpectrum(3, 17, 0.5, 0.9, "ws2");
    auto ws3 = pointDataWorkspaceMultiSpectrum(3, 19, 0.8, 1.3, "ws3");
    Stitch alg;
    alg.setRethrows(true);
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspaces", std::vector<std::string>({"ws1", "ws2", "ws3"})));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("TieScaleFactors", true));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OutputWorkspace", "out"));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
  }

private:
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
};
