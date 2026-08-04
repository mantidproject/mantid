// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidReflectometry/FindReflectometryLines2.h"

#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidHistogramData/LinearGenerator.h"

#include <array>
#include <limits>

using namespace Mantid;

class FindReflectometryLines2Test : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static FindReflectometryLines2Test *createSuite() { return new FindReflectometryLines2Test(); }
  static void destroySuite(FindReflectometryLines2Test *suite) { delete suite; }

  FindReflectometryLines2Test() : CxxTest::TestSuite() { API::FrameworkManager::Instance(); }

  void test_init() {
    Reflectometry::FindReflectometryLines2 alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_simplePeakSucceeds() {
    constexpr size_t nBins{256};
    constexpr size_t nHisto{128};
    auto ws = emptyWorkspace(nHisto, nBins);
    constexpr double verticalCentre = static_cast<double>(nHisto) / 3.4;
    constexpr double verticalWidth = static_cast<double>(nHisto) / 20.;
    double const horizontalCentre = ws->x(0).back() / 1.5;
    double const horizontalWidth = ws->x(0).back() / 2.5;
    addReflectometryLine(*ws, horizontalCentre, horizontalWidth, verticalCentre, verticalWidth);
    Reflectometry::FindReflectometryLines2 alg;
    alg.setChild(true);
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", ws))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "_unused_for_child"))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())
    API::MatrixWorkspace_sptr outputWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outputWS);
    double const lineCentre = alg.getProperty("LineCentre");
    TS_ASSERT_EQUALS(outputWS->y(0)[0], lineCentre)
    TS_ASSERT_DELTA(lineCentre, verticalCentre, 1e-8)
  }

  void test_StartEndWorkspaceIndicesWithTwoPeaks() {
    constexpr size_t nBins{256};
    constexpr size_t nHisto{128};
    auto ws = emptyWorkspace(nHisto, nBins);
    std::array<double, 2> const verticalCentres{
        {static_cast<double>(nHisto) / 3., static_cast<double>(nHisto) * 2. / 3.}};
    constexpr double verticalWidth = static_cast<double>(nHisto) / 20.;
    double const horizontalCentre = ws->x(0).back() / 1.5;
    double const horizontalWidth = ws->x(0).back() / 2.5;
    addReflectometryLine(*ws, horizontalCentre, horizontalWidth, verticalCentres.front(), verticalWidth);
    addReflectometryLine(*ws, horizontalCentre, horizontalWidth, verticalCentres.back(), verticalWidth);
    for (auto const centre : verticalCentres) {
      Reflectometry::FindReflectometryLines2 alg;
      alg.setChild(true);
      alg.setRethrows(true);
      TS_ASSERT_THROWS_NOTHING(alg.initialize())
      TS_ASSERT(alg.isInitialized())
      TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", ws))
      TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "_unused_for_child"))
      TS_ASSERT_THROWS_NOTHING(alg.setProperty("StartWorkspaceIndex", static_cast<int>(centre - 2 * verticalWidth)))
      TS_ASSERT_THROWS_NOTHING(alg.setProperty("EndWorkspaceIndex", static_cast<int>(centre + 2 * verticalWidth)))
      TS_ASSERT_THROWS_NOTHING(alg.execute())
      TS_ASSERT(alg.isExecuted())
      API::MatrixWorkspace_sptr outputWS = alg.getProperty("OutputWorkspace");
      TS_ASSERT(outputWS);
      double const lineCentre = alg.getProperty("LineCentre");
      TS_ASSERT_EQUALS(outputWS->y(0)[0], lineCentre)
      TS_ASSERT_DELTA(lineCentre, centre, 1e-8)
    }
  }

  void test_RangeLowerAndUpperWithTwoPeaks() {
    constexpr size_t nBins{256};
    constexpr size_t nHisto{128};
    auto ws = emptyWorkspace(nHisto, nBins);
    std::array<double, 2> const verticalCentres{
        {static_cast<double>(nHisto) / 3., static_cast<double>(nHisto) * 2. / 3.}};
    constexpr double verticalWidth = static_cast<double>(nHisto) / 20.;
    std::array<double, 2> const horizontalCentres{{ws->x(0).back() / 4., ws->x(0).back() * 3. / 4.}};
    double const horizontalWidth = ws->x(0).back() / 6.;
    for (size_t i = 0; i < 2; ++i) {
      addReflectometryLine(*ws, horizontalCentres[i], horizontalWidth, verticalCentres[i], verticalWidth);
      addReflectometryLine(*ws, horizontalCentres[i], horizontalWidth, verticalCentres[i], verticalWidth);
    }
    std::array<double, 2> const lower{{0., 0.5 * ws->x(0).back()}};
    std::array<double, 2> const upper{{0.5 * ws->x(0).back(), ws->x(0).back()}};
    for (size_t i = 0; i < 2; ++i) {
      Reflectometry::FindReflectometryLines2 alg;
      alg.setChild(true);
      alg.setRethrows(true);
      TS_ASSERT_THROWS_NOTHING(alg.initialize())
      TS_ASSERT(alg.isInitialized())
      TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", ws))
      TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "_unused_for_child"))
      TS_ASSERT_THROWS_NOTHING(alg.setProperty("RangeLower", lower[i]))
      TS_ASSERT_THROWS_NOTHING(alg.setProperty("RangeUpper", upper[i]))
      TS_ASSERT_THROWS_NOTHING(alg.execute())
      TS_ASSERT(alg.isExecuted())
      API::MatrixWorkspace_sptr outputWS = alg.getProperty("OutputWorkspace");
      TS_ASSERT(outputWS);
      double const lineCentre = alg.getProperty("LineCentre");
      TS_ASSERT_EQUALS(outputWS->y(0)[0], lineCentre)
      TS_ASSERT_DELTA(lineCentre, verticalCentres[i], 1e-8)
    }
  }

  void test_invalidRangeLowerAndUpperThrows() {
    constexpr size_t nBins{256};
    constexpr size_t nHisto{128};
    auto ws = emptyWorkspace(nHisto, nBins);
    Reflectometry::FindReflectometryLines2 alg;
    alg.setChild(true);
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", ws))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "_unused_for_child"))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("RangeLower", 2.))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("RangeUpper", 1.))
    std::string err_msg("Some invalid Properties found: \n"
                        " RangeUpper: The upper limit is smaller than the lower.");
    TS_ASSERT_THROWS_EQUALS(alg.execute(), std::runtime_error const &e, e.what(), err_msg)
  }

  void test_invalidEndAndStartIndicesThrows() {
    constexpr size_t nBins{256};
    constexpr size_t nHisto{128};
    auto ws = emptyWorkspace(nHisto, nBins);
    Reflectometry::FindReflectometryLines2 alg;
    alg.setChild(true);
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", ws))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "_unused_for_child"))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("StartWorkspaceIndex", 2))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("EndWorkspaceIndex", 1))
    std::string err_msg("Some invalid Properties found: \n"
                        " EndWorkspaceIndex: The index is smaller than the start.");
    TS_ASSERT_THROWS_EQUALS(alg.execute(), std::runtime_error const &e, e.what(), err_msg);
  }

  void test_flat_profile_returns_absolute_maximum_and_optional_output_workspace() {
    auto ws = emptyWorkspace(20, 10);
    for (size_t workspaceIndex = 0; workspaceIndex < ws->getNumberHistograms(); ++workspaceIndex) {
      std::fill(ws->mutableY(workspaceIndex).begin(), ws->mutableY(workspaceIndex).end(), 2.0);
    }
    Reflectometry::FindReflectometryLines2 alg;
    alg.setChild(true);
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", ws);
    alg.setProperty("StartWorkspaceIndex", 4);
    alg.setPropertyValue("OutputWorkspace", "unused_output");

    TS_ASSERT_THROWS_NOTHING(alg.execute())
    double const lineCentre = alg.getProperty("LineCentre");
    API::MatrixWorkspace_sptr output = alg.getProperty("OutputWorkspace");
    TS_ASSERT_EQUALS(lineCentre, 4.0)
    TS_ASSERT(output)
    if (output) {
      TS_ASSERT_EQUALS(output->y(0)[0], 4.0)
    }
  }

  void test_profile_without_a_finite_maximum_throws() {
    auto ws = emptyWorkspace(20, 10);
    for (size_t workspaceIndex = 0; workspaceIndex < ws->getNumberHistograms(); ++workspaceIndex) {
      std::fill(ws->mutableY(workspaceIndex).begin(), ws->mutableY(workspaceIndex).end(),
                std::numeric_limits<double>::quiet_NaN());
    }
    Reflectometry::FindReflectometryLines2 alg;
    alg.setChild(true);
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", ws);

    TS_ASSERT_THROWS(alg.execute(), std::runtime_error const &)
  }

private:
  static void addReflectometryLine(API::MatrixWorkspace &ws, double const horizontalCentre,
                                   double const horizontalWidth, double const verticalCentre,
                                   double const verticalWidth) {
    auto verticalFunction = [verticalCentre, verticalWidth](double const x) {
      // Simple Gaussian ___.-^-.___
      auto const arg = (x - verticalCentre) / verticalWidth;
      return std::exp(-arg * arg);
    };
    auto horizontalFunction = [horizontalCentre, horizontalWidth](double const x) {
      // Ugly box function ___|^^^^|___
      if (x >= horizontalCentre - horizontalWidth / 2. && x < horizontalCentre + horizontalWidth / 2.) {
        return 1.;
      } else {
        return 0.;
      }
    };
    for (size_t wsIndex = 0; wsIndex < ws.getNumberHistograms(); ++wsIndex) {
      auto const &Xs = ws.x(wsIndex);
      auto &Ys = ws.mutableY(wsIndex);
      auto &Es = ws.mutableE(wsIndex);
      for (size_t binIndex = 0; binIndex < Ys.size(); ++binIndex) {
        auto const y = verticalFunction(static_cast<double>(wsIndex)) * horizontalFunction(Xs[binIndex]);
        Ys[binIndex] += y;
        auto const e = Es[binIndex];
        Es[binIndex] = std::sqrt(e * e + y);
      }
    }
  }

  static API::MatrixWorkspace_sptr emptyWorkspace(size_t const nHisto, size_t const nBins) {
    HistogramData::BinEdges const edges{nBins + 1, HistogramData::LinearGenerator(0., 2.3)};
    HistogramData::Counts const counts(nBins, 0.);
    API::MatrixWorkspace_sptr ws =
        DataObjects::create<DataObjects::Workspace2D>(nHisto, HistogramData::Histogram(edges, counts));
    return ws;
  }
};
