// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAlgorithms/Stitch.h"
#include "MantidHistogramData/Histogram.h"

using Mantid::Algorithms::Stitch;
using namespace Mantid::API;
using namespace Mantid::HistogramData;

class StitchTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static StitchTest *createSuite() { return new StitchTest(); }
  static void destroySuite(StitchTest *suite) { delete suite; }

  void test_Init() {
    Stitch alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_NoOverlap() {}

  void test_OneWorkspace() {}

  void test_WorkspaceGroup() {}

  void test_WorkspacesAndGroupsMixed() {}

  void test_IncompatibleWorkspaces() {}

  void test_NotEnoughOverlap() {}

  void test_NoExplicitReference() {}

  void test_ExplicitReference() {}

  void test_PointData() {}

  void test_HistogramData() {}

  void test_MultipleSpectra() {}

  void test_LeftToRight() {}

  void test_RightToLeft() {}

  void test_CustomOrder() {}

private:
  MatrixWorkspace_sptr sampleWorkspaceOneSpec(size_t nBins, double startX, double endX) {
    MatrixWorkspace_sptr ws = WorkspaceFactory::Instance().create("Workspace2D", 1, nBins, nBins);
    std::vector<double> x(nBins + 1), y(nBins), e(nBins);
    const double step = (startX - endX) / nBins;
    for (size_t ibin = 0; ibin < nBins; ++ibin) {
      x[ibin] = startX + ibin * step;
      y[ibin] = 7 * ibin + 3;
      e[ibin] = std::sqrt(y[ibin]);
    }
    x[nBins] = endX;
    ws->setHistogram(0, Histogram(x, y, e));
    return ws;
  }
};
