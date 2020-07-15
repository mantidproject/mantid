// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAlgorithms/SampleCorrections/SparseInstrument.h"

#include "MantidAPI/SpectrumInfo.h"
#include "MantidAlgorithms/SampleCorrections/DetectorGridDefinition.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidHistogramData/Histogram.h"
#include "MantidHistogramData/LinearGenerator.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include <cxxtest/TestSuite.h>

using namespace Mantid::Algorithms;
using namespace Mantid::Geometry;

// allow testing of protected methods. This is in part to provide some
// continuity from previous versions where all of the methods were free
// functions
class SparseWorkspaceTest : public SparseWorkspace {
public:
  SparseWorkspaceTest(const Mantid::API::MatrixWorkspace &modelWS,
                      const size_t wavelengthPoints, const size_t rows,
                      const size_t columns)
      : SparseWorkspace(modelWS, wavelengthPoints, rows, columns){};
  static std::array<double, 4>
  inverseDistanceWeights(const std::array<double, 4> &distances) {
    return SparseWorkspace::inverseDistanceWeights(distances);
  };
  static double greatCircleDistance(const double lat1, const double long1,
                                    const double lat2, const double long2) {
    return SparseWorkspace::greatCircleDistance(lat1, long1, lat2, long2);
  };
  std::unique_ptr<DetectorGridDefinition> grid() {
    return std::move(m_gridDef);
  }
};

class SparseInstrumentTest : public CxxTest::TestSuite {
public:
  static SparseInstrumentTest *createSuite() {
    return new SparseInstrumentTest();
  }

  static void destroySuite(SparseInstrumentTest *suite) { delete suite; }

  SparseInstrumentTest() {}

  void test_createSparseWS() {
    using namespace WorkspaceCreationHelper;
    auto ws = create2DWorkspaceWithRectangularInstrument(1, 2, 10);
    const size_t gridRows = 5;
    const size_t gridCols = 3;
    const size_t wavelengths = 3;
    auto sparseWS =
        std::make_unique<SparseWorkspace>(*ws, wavelengths, gridRows, gridCols);
    TS_ASSERT_EQUALS(sparseWS->getNumberHistograms(), gridRows * gridCols)
    TS_ASSERT_EQUALS(sparseWS->blocksize(), wavelengths)
    const auto p = ws->points(0);
    for (size_t i = 0; i < sparseWS->getNumberHistograms(); ++i) {
      const auto sparseP = sparseWS->points(i);
      TS_ASSERT_EQUALS(sparseP.front(), p.front())
      TS_ASSERT_EQUALS(sparseP.back(), p.back())
    }
    double minLat;
    double maxLat;
    double minLon;
    double maxLon;
    std::tie(minLat, maxLat, minLon, maxLon) =
        ws->spectrumInfo().extremeAngles();
    double sparseMinLat;
    double sparseMaxLat;
    double sparseMinLon;
    double sparseMaxLon;
    std::tie(sparseMinLat, sparseMaxLat, sparseMinLon, sparseMaxLon) =
        sparseWS->spectrumInfo().extremeAngles();
    TS_ASSERT_EQUALS(sparseMinLat, minLat)
    TS_ASSERT_DELTA(sparseMaxLat, maxLat, 1e-8)
    TS_ASSERT_EQUALS(sparseMinLon, minLon)
    TS_ASSERT_DELTA(sparseMaxLon, maxLon, 1e-8)
  }

  void test_extremeWavelengths_binEdgeData() {
    using namespace Mantid::DataObjects;
    using namespace Mantid::HistogramData;
    const BinEdges edges{-1.0, 2.0, 4.0};
    const Counts counts{0.0, 0.0};
    auto ws = create<Workspace2D>(2, Histogram(edges, counts));
    ws->mutableX(1) = {-3.0, -1.0, 1.0};
    double minWavelength, maxWavelength;
    auto sparseWS = std::make_unique<SparseWorkspace>(*ws, 3, 2, 1);

    minWavelength = sparseWS->dataX(0)[0];
    maxWavelength = sparseWS->dataX(0)[sparseWS->blocksize() - 1];
    TS_ASSERT_EQUALS(minWavelength, -2.0)
    TS_ASSERT_EQUALS(maxWavelength, 3.0)
  }

  void test_extremeWavelengths_pointData() {
    using namespace Mantid::DataObjects;
    using namespace Mantid::HistogramData;
    const Points edges{-1.0, 2.0, 4.0};
    const Counts counts{0.0, 0.0, 0.0};
    auto ws = create<Workspace2D>(2, Histogram(edges, counts));
    ws->mutableX(1) = {-3.0, -1.0, 1.0};
    double minWavelength, maxWavelength;
    auto sparseWS = std::make_unique<SparseWorkspace>(*ws, 3, 2, 1);

    minWavelength = sparseWS->dataX(0)[0];
    maxWavelength = sparseWS->dataX(0)[sparseWS->blocksize() - 1];
    TS_ASSERT_EQUALS(minWavelength, -3.0)
    TS_ASSERT_EQUALS(maxWavelength, 4.0)
  }

  void test_createDetectorGridDefinition_multipleDetectors() {
    using namespace WorkspaceCreationHelper;
    auto ws = create2DWorkspaceWithRectangularInstrument(1, 2, 1);
    const size_t gridRows = 3;
    const size_t gridCols = 4;
    auto sparseWS =
        std::make_unique<SparseWorkspace>(*ws, 1, gridRows, gridCols);

    const auto &spectrumInfo = ws->spectrumInfo();

    double lat;
    double lon;
    std::tie(lat, lon) = spectrumInfo.geographicalAngles(0);
    double sparseLat;
    double sparseLon;
    std::tie(sparseLat, sparseLon) =
        sparseWS->spectrumInfo().geographicalAngles(0);
    TS_ASSERT_EQUALS(sparseLat, lat)
    TS_ASSERT_EQUALS(sparseLon, lon)

    std::tie(lat, lon) = spectrumInfo.geographicalAngles(3);
    std::tie(sparseLat, sparseLon) =
        sparseWS->spectrumInfo().geographicalAngles(3);
    TS_ASSERT_DELTA(sparseLat, lat, 1e-8)
    TS_ASSERT_EQUALS(sparseLon, lon)
  }

  void test_createDetectorGridDefinition_singleDetector() {
    using namespace WorkspaceCreationHelper;
    auto ws = create2DWorkspaceWithRectangularInstrument(1, 1, 1);
    double lat;
    double lon;
    std::tie(lat, lon) = ws->spectrumInfo().geographicalAngles(0);
    auto sparseWS = std::make_unique<SparseWorkspace>(*ws, 1, 2, 2);

    double sparseLat;
    double sparseLon;
    std::tie(sparseLat, sparseLon) =
        sparseWS->spectrumInfo().geographicalAngles(0);
    TS_ASSERT_LESS_THAN(sparseLat, lat)
    TS_ASSERT_LESS_THAN(sparseLon, lon)

    std::tie(sparseLat, sparseLon) =
        sparseWS->spectrumInfo().geographicalAngles(0);
    TS_ASSERT_LESS_THAN(lat, sparseLat)
    TS_ASSERT_LESS_THAN(lon, sparseLon)
  }

  void test_modelHistogram_coversModelWS() {
    using namespace Mantid::DataObjects;
    using namespace Mantid::HistogramData;

    const BinEdges edges(256, LinearGenerator(-1.33, 0.77));
    const Counts counts(edges.size() - 1, 0.);
    auto ws = create<Workspace2D>(2, Histogram(edges, counts));
    const auto points = ws->points(0);
    for (size_t nCounts = 2; nCounts < counts.size(); ++nCounts) {
      const auto sparseWS =
          std::make_unique<SparseWorkspace>(*ws, nCounts, 2, 1);
      const auto histo = sparseWS->histogram(0);

      TS_ASSERT_EQUALS(sparseWS->blocksize(), nCounts)
      // Check the stepping inside modelHistogram retains the final wavelength
      // point as returned by extremeWavelengths w/o rounding errors
      // These have to be equal, don't use DELTA here!
      TS_ASSERT_EQUALS(histo.x().front(), points.front())
      TS_ASSERT_EQUALS(histo.x().back(), points.back())
    }
  }

  void test_greatCircleDistance() {
    double d = SparseWorkspaceTest::greatCircleDistance(0, 0, 0, 0);
    TS_ASSERT_EQUALS(d, 0.0);
    d = SparseWorkspaceTest::greatCircleDistance(M_PI / 2, 0.0, -M_PI / 2, 0.0);
    TS_ASSERT_EQUALS(d, M_PI)
    d = SparseWorkspaceTest::greatCircleDistance(M_PI / 4, M_PI / 4, -M_PI / 4,
                                                 -M_PI / 4);
    TS_ASSERT_DELTA(d, 2 * M_PI / 3, 1e-8)
  }

  void test_interpolateFromDetectorGrid() {
    using namespace WorkspaceCreationHelper;
    auto ws = create2DWorkspaceWithRectangularInstrument(1, 2, 7);
    const size_t sparseRows = 3;
    const size_t sparseCols = 6;
    const size_t wavelengths = 3;
    auto sparseWS = std::make_unique<SparseWorkspaceTest>(
        *ws, wavelengths, sparseRows, sparseCols);
    for (size_t i = 0; i < sparseWS->getNumberHistograms(); ++i) {
      auto &ys = sparseWS->mutableY(i);
      auto &es = sparseWS->mutableE(i);
      for (size_t j = 0; j < ys.size(); ++j) {
        ys[j] = static_cast<double>(i);
        es[j] = std::sqrt(ys[j]);
      }
    }
    auto &grid = sparseWS->grid();
    double lat = grid->latitudeAt(0);
    double lon = grid->longitudeAt(0);
    auto indices = grid->nearestNeighbourIndices(lat, lon);
    auto h = sparseWS->interpolateFromDetectorGrid(lat, lon);
    TS_ASSERT_EQUALS(h.size(), wavelengths)
    for (size_t i = 0; i < h.size(); ++i) {
      TS_ASSERT_EQUALS(h.y()[i], 0.0)
      TS_ASSERT_EQUALS(h.e()[i], 0.0)
    }
    lat = (grid->latitudeAt(2) + grid->latitudeAt(1)) / 2.0;
    lon = (grid->longitudeAt(3) + grid->longitudeAt(2)) / 2.0;
    indices = sparseWS->grid()->nearestNeighbourIndices(lat, lon);
    double val =
        static_cast<double>(indices[0] + indices[1] + indices[2] + indices[3]) /
        4.0;
    h = sparseWS->interpolateFromDetectorGrid(lat, lon);
    TS_ASSERT_EQUALS(h.size(), wavelengths)
    for (size_t i = 0; i < h.size(); ++i) {
      TS_ASSERT_DELTA(h.y()[i], val, 1e-7)
      TS_ASSERT_EQUALS(h.e()[i], 0.0)
    }
  }

  void test_inverseDistanceWeights() {
    std::array<double, 4> ds{{0.3, 0.3, 0.0, 0.3}};
    auto weights = SparseWorkspaceTest::inverseDistanceWeights(ds);
    TS_ASSERT_EQUALS(weights[0], 0.0)
    TS_ASSERT_EQUALS(weights[1], 0.0)
    TS_ASSERT_EQUALS(weights[2], 1.0)
    TS_ASSERT_EQUALS(weights[3], 0.0)
    ds = {{0.2, 0.3, 0.1, 0.4}};
    weights = SparseWorkspaceTest::inverseDistanceWeights(ds);
    TS_ASSERT_EQUALS(weights[0], 1 / 0.2 / 0.2)
    TS_ASSERT_EQUALS(weights[1], 1 / 0.3 / 0.3)
    TS_ASSERT_EQUALS(weights[2], 1 / 0.1 / 0.1)
    TS_ASSERT_EQUALS(weights[3], 1 / 0.4 / 0.4)
  }
};
