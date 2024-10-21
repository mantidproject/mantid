// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAlgorithms/SampleCorrections/SparseWorkspace.h"

#include "MantidAPI/SpectrumInfo.h"
#include "MantidAlgorithms/SampleCorrections/DetectorGridDefinition.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidHistogramData/Histogram.h"
#include "MantidHistogramData/LinearGenerator.h"

#include <cxxtest/TestSuite.h>

using namespace Mantid::Algorithms;
using namespace Mantid::Geometry;

// allow testing of protected methods. This is in part to provide some
// continuity from previous versions where all of the methods were free
// functions
class SparseWorkspaceWrapper : public SparseWorkspace {
public:
  SparseWorkspaceWrapper(const Mantid::API::MatrixWorkspace &modelWS, const size_t wavelengthPoints, const size_t rows,
                         const size_t columns)
      : SparseWorkspace(modelWS, wavelengthPoints, rows, columns) {};
  static std::array<double, 4> inverseDistanceWeights(const std::array<double, 4> &distances) {
    return SparseWorkspace::inverseDistanceWeights(distances);
  };
  static double greatCircleDistance(const double lat1, const double long1, const double lat2, const double long2) {
    return SparseWorkspace::greatCircleDistance(lat1, long1, lat2, long2);
  };
  DetectorGridDefinition &grid() { return *m_gridDef; }
  static std::tuple<double, double> extremeWavelengths(const Mantid::API::MatrixWorkspace &ws) {
    return SparseWorkspace::extremeWavelengths(ws);
  };
  static Mantid::HistogramData::Histogram modelHistogram(const Mantid::API::MatrixWorkspace &modelWS,
                                                         const size_t wavelengthPoints) {
    return SparseWorkspace::modelHistogram(modelWS, wavelengthPoints);
  };
  static std::tuple<double, double, double, double> extremeAngles(const Mantid::API::MatrixWorkspace &ws) {
    return SparseWorkspace::extremeAngles(ws);
  }
  Mantid::HistogramData::HistogramY secondDerivative(const std::array<size_t, 3> indices, const double distanceStep) {
    return SparseWorkspace::secondDerivative(indices, distanceStep);
  }
};

class SparseWorkspaceTest : public CxxTest::TestSuite {
public:
  static SparseWorkspaceTest *createSuite() { return new SparseWorkspaceTest(); }

  static void destroySuite(SparseWorkspaceTest *suite) { delete suite; }

  SparseWorkspaceTest() {}

  void test_createSparseWS() {
    using namespace WorkspaceCreationHelper;
    auto ws = create2DWorkspaceWithRectangularInstrument(1, 2, 10);
    constexpr size_t gridRows = 5;
    constexpr size_t gridCols = 3;
    constexpr size_t wavelengths = 3;
    auto sparseWS = std::make_unique<SparseWorkspace>(*ws, wavelengths, gridRows, gridCols);
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
    std::tie(minLat, maxLat, minLon, maxLon) = SparseWorkspaceWrapper::extremeAngles(*ws);
    double sparseMinLat;
    double sparseMaxLat;
    double sparseMinLon;
    double sparseMaxLon;
    std::tie(sparseMinLat, sparseMaxLat, sparseMinLon, sparseMaxLon) = SparseWorkspaceWrapper::extremeAngles(*ws);
    TS_ASSERT_EQUALS(sparseMinLat, minLat)
    TS_ASSERT_DELTA(sparseMaxLat, maxLat, 1e-8)
    TS_ASSERT_EQUALS(sparseMinLon, minLon)
    TS_ASSERT_DELTA(sparseMaxLon, maxLon, 1e-8)
  }

  void test_extremeAngles_multipleDetectors() {
    using namespace WorkspaceCreationHelper;
    auto ws = create2DWorkspaceWithRectangularInstrument(1, 2, 1);
    const auto &spectrumInfo = ws->spectrumInfo();
    double minLat;
    double minLon;
    double maxLat;
    double maxLon;
    std::tie(minLat, maxLat, minLon, maxLon) = SparseWorkspaceWrapper::extremeAngles(*ws);
    for (size_t i = 0; i < ws->getNumberHistograms(); ++i) {
      double lat;
      double lon;
      std::tie(lat, lon) = spectrumInfo.geographicalAngles(i);
      TS_ASSERT_LESS_THAN_EQUALS(minLat, lat)
      TS_ASSERT_LESS_THAN_EQUALS(minLon, lon)
      TS_ASSERT_LESS_THAN_EQUALS(lat, maxLat)
      TS_ASSERT_LESS_THAN_EQUALS(lon, maxLon)
    }
  }

  void test_extremeAngles_singleDetector() {
    using namespace WorkspaceCreationHelper;
    auto ws = create2DWorkspaceWithRectangularInstrument(1, 1, 1);
    double minLat;
    double minLon;
    double maxLat;
    double maxLon;
    std::tie(minLat, maxLat, minLon, maxLon) = SparseWorkspaceWrapper::extremeAngles(*ws);
    TS_ASSERT_EQUALS(minLat, 0)
    TS_ASSERT_EQUALS(minLon, 0)
    TS_ASSERT_EQUALS(maxLat, 0)
    TS_ASSERT_EQUALS(maxLon, 0)
  }

  void test_extremeWavelengths_binEdgeData() {
    using namespace Mantid::DataObjects;
    using namespace Mantid::HistogramData;
    const BinEdges edges{-1.0, 2.0, 4.0};
    const Counts counts{0.0, 0.0};
    auto ws = create<Workspace2D>(2, Histogram(edges, counts));
    ws->mutableX(1) = {-3.0, -1.0, 1.0};
    double minWavelength, maxWavelength;
    std::tie(minWavelength, maxWavelength) = SparseWorkspaceWrapper::extremeWavelengths(*ws);
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
    std::tie(minWavelength, maxWavelength) = SparseWorkspaceWrapper::extremeWavelengths(*ws);
    TS_ASSERT_EQUALS(minWavelength, -3.0)
    TS_ASSERT_EQUALS(maxWavelength, 4.0)
  }

  void test_createDetectorGridDefinition_multipleDetectors() {
    using namespace WorkspaceCreationHelper;
    auto ws = create2DWorkspaceWithRectangularInstrument(1, 2, 1);
    constexpr size_t gridRows = 3;
    constexpr size_t gridCols = 4;
    auto sparseWS = std::make_unique<SparseWorkspace>(*ws, 1, gridRows, gridCols);

    const auto &spectrumInfo = ws->spectrumInfo();

    double lat;
    double lon;
    std::tie(lat, lon) = spectrumInfo.geographicalAngles(0);
    double sparseLat;
    double sparseLon;
    std::tie(sparseLat, sparseLon) = sparseWS->spectrumInfo().geographicalAngles(0);
    TS_ASSERT_EQUALS(sparseLat, lat)
    TS_ASSERT_EQUALS(sparseLon, lon)

    std::tie(lat, lon) = spectrumInfo.geographicalAngles(3);
    std::tie(sparseLat, sparseLon) = sparseWS->spectrumInfo().geographicalAngles(11);
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
    std::tie(sparseLat, sparseLon) = sparseWS->spectrumInfo().geographicalAngles(0);
    TS_ASSERT_LESS_THAN(sparseLat, lat)
    TS_ASSERT_LESS_THAN(sparseLon, lon)

    std::tie(sparseLat, sparseLon) = sparseWS->spectrumInfo().geographicalAngles(3);
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
      const auto histo = SparseWorkspaceWrapper::modelHistogram(*ws, nCounts);

      // Check the stepping inside modelHistogram retains the final wavelength
      // point as returned by extremeWavelengths w/o rounding errors
      // These have to be equal, don't use DELTA here!
      TS_ASSERT_EQUALS(histo.x().front(), points.front())
      TS_ASSERT_EQUALS(histo.x().back(), points.back())
    }
  }

  void test_greatCircleDistance() {
    double d = SparseWorkspaceWrapper::greatCircleDistance(0, 0, 0, 0);
    TS_ASSERT_EQUALS(d, 0.0);
    d = SparseWorkspaceWrapper::greatCircleDistance(M_PI / 2, 0.0, -M_PI / 2, 0.0);
    TS_ASSERT_EQUALS(d, M_PI)
    d = SparseWorkspaceWrapper::greatCircleDistance(M_PI / 4, M_PI / 4, -M_PI / 4, -M_PI / 4);
    TS_ASSERT_DELTA(d, 2 * M_PI / 3, 1e-8)
  }

  void test_interpolateFromDetectorGrid_OnSparseDetector() {
    using namespace WorkspaceCreationHelper;
    auto ws = create2DWorkspaceWithRectangularInstrument(1, 2, 7);
    const size_t sparseRows = 3;
    const size_t sparseCols = 6;
    const size_t wavelengths = 3;
    auto sparseWS = std::make_unique<SparseWorkspaceWrapper>(*ws, wavelengths, sparseRows, sparseCols);
    for (size_t i = 0; i < sparseWS->getNumberHistograms(); ++i) {
      auto &ys = sparseWS->mutableY(i);
      auto &es = sparseWS->mutableE(i);
      for (size_t j = 0; j < ys.size(); ++j) {
        ys[j] = static_cast<double>(i);
        es[j] = std::sqrt(ys[j]);
      }
    }
    auto &grid = sparseWS->grid();
    double lat = grid.latitudeAt(0);
    double lon = grid.longitudeAt(0);
    auto h = sparseWS->interpolateFromDetectorGrid(lat, lon);
    TS_ASSERT_EQUALS(h.size(), wavelengths)
    for (size_t i = 0; i < h.size(); ++i) {
      TS_ASSERT_EQUALS(h.y()[i], 0.0)
      TS_ASSERT_EQUALS(h.e()[i], 0.0)
    }
    lat = (grid.latitudeAt(2) + grid.latitudeAt(1)) / 2.0;
    lon = (grid.longitudeAt(3) + grid.longitudeAt(2)) / 2.0;
    auto indices = sparseWS->grid().nearestNeighbourIndices(lat, lon);
    double val = static_cast<double>(indices[0] + indices[1] + indices[2] + indices[3]) / 4.0;
    h = sparseWS->interpolateFromDetectorGrid(lat, lon);
    TS_ASSERT_EQUALS(h.size(), wavelengths)
    for (size_t i = 0; i < h.size(); ++i) {
      TS_ASSERT_DELTA(h.y()[i], val, 1e-7)
      TS_ASSERT_EQUALS(h.e()[i], 0.0)
    }
  }

  void test_interpolateFromDetectorGrid_InBetweenSparseDetectors() {
    using namespace WorkspaceCreationHelper;
    auto ws = create2DWorkspaceWithRectangularInstrument(1, 2, 7);
    const size_t sparseRows = 3;
    const size_t sparseCols = 6;
    const size_t wavelengths = 3;
    auto sparseWS = std::make_unique<SparseWorkspaceWrapper>(*ws, wavelengths, sparseRows, sparseCols);
    for (size_t i = 0; i < sparseWS->getNumberHistograms(); ++i) {
      auto &ys = sparseWS->mutableY(i);
      auto &es = sparseWS->mutableE(i);
      for (size_t j = 0; j < ys.size(); ++j) {
        ys[j] = static_cast<double>(i);
        es[j] = std::sqrt(ys[j]);
      }
    }
    auto &grid = sparseWS->grid();
    double lat = grid.latitudeAt(1);
    double lon = (grid.longitudeAt(3) + grid.longitudeAt(2)) / 2.0;
    auto indices = sparseWS->grid().nearestNeighbourIndices(lat, lon);
    double distance1 = sparseWS->greatCircleDistance(lat, lon, grid.latitudeAt(1), grid.longitudeAt(2));
    double distance2 = sparseWS->greatCircleDistance(lat, lon, grid.latitudeAt(2), grid.longitudeAt(2));
    double sumWeights = 2 / pow(distance1, 2) + 2 / pow(distance2, 2);
    double val = (static_cast<double>(indices[0] + indices[2]) / pow(distance1, 2) +
                  static_cast<double>(indices[1] + indices[3]) / pow(distance2, 2)) /
                 sumWeights;
    auto h = sparseWS->interpolateFromDetectorGrid(lat, lon);
    TS_ASSERT_EQUALS(h.size(), wavelengths)
    for (size_t i = 0; i < h.size(); ++i) {
      TS_ASSERT_DELTA(h.y()[i], val, 1e-7)
      TS_ASSERT_EQUALS(h.e()[i], 0.0)
    }
  }

  // test the interpolation and the error propagation
  void test_bilinearInterpolateFromDetectorGrid_OnSparseDetector() {
    using namespace WorkspaceCreationHelper;
    auto ws = create2DWorkspaceWithRectangularInstrument(1, 2, 7);
    const size_t sparseRows = 3;
    const size_t sparseCols = 6;
    const size_t wavelengths = 3;
    auto sparseWS = std::make_unique<SparseWorkspaceWrapper>(*ws, wavelengths, sparseRows, sparseCols);
    for (size_t i = 0; i < sparseWS->getNumberHistograms(); ++i) {
      auto &ys = sparseWS->mutableY(i);
      auto &es = sparseWS->mutableE(i);
      for (size_t j = 0; j < ys.size(); ++j) {
        ys[j] = static_cast<double>(i);
        es[j] = std::sqrt(ys[j]);
      }
    }
    auto &grid = sparseWS->grid();
    double lat = grid.latitudeAt(0);
    double lon = grid.longitudeAt(0);
    auto h = sparseWS->bilinearInterpolateFromDetectorGrid(lat, lon);
    TS_ASSERT_EQUALS(h.size(), wavelengths)
    for (size_t i = 0; i < h.size(); ++i) {
      TS_ASSERT_EQUALS(h.y()[i], 0.0)
      TS_ASSERT_EQUALS(h.e()[i], 0.0)
    }
  }

  void test_bilinearInterpolateFromDetectorGrid_InBetweenSparseDetectors() {
    using namespace WorkspaceCreationHelper;
    auto ws = create2DWorkspaceWithRectangularInstrument(1, 2, 7);
    const size_t sparseRows = 3;
    const size_t sparseCols = 6;
    const size_t wavelengths = 3;
    auto sparseWS = std::make_unique<SparseWorkspaceWrapper>(*ws, wavelengths, sparseRows, sparseCols);
    for (size_t i = 0; i < sparseWS->getNumberHistograms(); ++i) {
      auto &ys = sparseWS->mutableY(i);
      auto &es = sparseWS->mutableE(i);
      for (size_t j = 0; j < ys.size(); ++j) {
        ys[j] = static_cast<double>(i);
        es[j] = std::sqrt(ys[j]);
      }
    }
    auto &grid = sparseWS->grid();
    double lat = (grid.latitudeAt(2) + grid.latitudeAt(1)) / 2.0;
    double lon = (grid.longitudeAt(3) + grid.longitudeAt(2)) / 2.0;
    size_t nearestLatIndex, nearestLonIndex;
    std::tie(nearestLatIndex, nearestLonIndex) = sparseWS->grid().getNearestVertex(lat, lon);
    double indexSum = static_cast<double>(sparseWS->grid().getDetectorIndex(nearestLatIndex, nearestLonIndex) +
                                          sparseWS->grid().getDetectorIndex(nearestLatIndex + 1, nearestLonIndex) +
                                          sparseWS->grid().getDetectorIndex(nearestLatIndex, nearestLonIndex + 1) +
                                          sparseWS->grid().getDetectorIndex(nearestLatIndex + 1, nearestLonIndex + 1));
    double val = indexSum / 4.0;
    // second derivative is zero here so error will be from propagating
    // the original errors on points only
    double err = sqrt(indexSum) / 4.0;

    auto h = sparseWS->bilinearInterpolateFromDetectorGrid(lat, lon);
    TS_ASSERT_EQUALS(h.size(), wavelengths)
    for (size_t i = 0; i < h.size(); ++i) {
      TS_ASSERT_DELTA(h.y()[i], val, 1e-7)
      TS_ASSERT_DELTA(h.e()[i], err, 1e-7);
    }
  }

  // test the interpolation error
  void test_bilinearInterpolateFromDetectorGrid_interpErrors_OnSparseDetector() {
    using namespace WorkspaceCreationHelper;
    auto ws = create2DWorkspaceWithRectangularInstrument(1, 2, 7);
    const size_t sparseRows = 3;
    const size_t sparseCols = 6;
    const size_t wavelengths = 3;
    auto sparseWS = std::make_unique<SparseWorkspaceWrapper>(*ws, wavelengths, sparseRows, sparseCols);
    for (size_t row = 0; row < sparseRows; row++) {
      for (size_t col = 0; col < sparseCols; col++) {
        auto &ys = sparseWS->mutableY(row + col * sparseRows);
        for (size_t j = 0; j < ys.size(); ++j) {
          ys[j] = std::pow(col, 2);
        }
      }
    }

    auto &grid = sparseWS->grid();
    double lat = grid.latitudeAt(0);
    double lon = grid.longitudeAt(0);
    auto h = sparseWS->bilinearInterpolateFromDetectorGrid(lat, lon);
    TS_ASSERT_EQUALS(h.size(), wavelengths)
    for (size_t i = 0; i < h.size(); ++i) {
      TS_ASSERT_EQUALS(h.y()[i], 0.0)
      TS_ASSERT_EQUALS(h.e()[i], 0.0)
    }
  }

  void test_bilinearInterpolateFromDetectorGrid_interpErrors_InBetweenSparseDetectors() {
    using namespace WorkspaceCreationHelper;
    auto ws = create2DWorkspaceWithRectangularInstrument(1, 2, 7);
    const size_t sparseRows = 3;
    const size_t sparseCols = 6;
    const size_t wavelengths = 3;
    auto sparseWS = std::make_unique<SparseWorkspaceWrapper>(*ws, wavelengths, sparseRows, sparseCols);
    for (size_t row = 0; row < sparseRows; row++) {
      for (size_t col = 0; col < sparseCols; col++) {
        auto &ys = sparseWS->mutableY(row + col * sparseRows);
        for (size_t j = 0; j < ys.size(); ++j) {
          ys[j] = std::pow(col, 2);
        }
      }
    }

    auto &grid = sparseWS->grid();
    const int longIndex = 2, latIndex = 1;
    double lat = (grid.latitudeAt(latIndex + 1) + grid.latitudeAt(latIndex)) / 2.0;
    double lon = (grid.longitudeAt(longIndex) + grid.longitudeAt(longIndex + 1)) / 2.0;
    double val = (pow(longIndex, 2) + pow(longIndex + 1, 2)) / 2.0;
    // 2nd derivative in long is 2.0, 2nd derivative in lat is zero
    double err = 0.5 * 0.5 * 0.5 * 2.0;
    auto h = sparseWS->bilinearInterpolateFromDetectorGrid(lat, lon);
    TS_ASSERT_EQUALS(h.size(), wavelengths)
    for (size_t i = 0; i < h.size(); ++i) {
      TS_ASSERT_DELTA(h.y()[i], val, 1e-7)
      TS_ASSERT_DELTA(h.e()[i], err, 1e-7)
    }

    // check error is positive even if 2nd deriv negative
    for (size_t row = 0; row < sparseRows; row++) {
      for (size_t col = 0; col < sparseCols; col++) {
        auto &ys = sparseWS->mutableY(row + col * sparseRows);
        for (size_t j = 0; j < ys.size(); ++j) {
          ys[j] = std::pow(sparseCols - 1, 2) - std::pow(sparseCols - col - 1, 2);
        }
      }
    }
    val = (2 * pow(sparseCols - 1, 2) - pow(sparseCols - longIndex - 1, 2) - pow(sparseCols - longIndex - 2, 2)) / 2.0;
    // 2nd derivative in long is -2.0, 2nd derivative in lat is zero
    err = 0.5 * 0.5 * 0.5 * 2.0;
    h = sparseWS->bilinearInterpolateFromDetectorGrid(lat, lon);
    TS_ASSERT_EQUALS(h.size(), wavelengths)
    for (size_t i = 0; i < h.size(); ++i) {
      TS_ASSERT_DELTA(h.y()[i], val, 1e-7)
      TS_ASSERT_DELTA(h.e()[i], err, 1e-7)
    }
  }

  void test_secondDerivative() {
    using namespace WorkspaceCreationHelper;
    auto ws = create2DWorkspaceWithRectangularInstrument(1, 4, 7);
    const int nDetectors = 3;
    std::array<size_t, nDetectors> indices = {1, 5, 9};
    const size_t wavelengths = 3;
    auto sparseWS = std::make_unique<SparseWorkspaceWrapper>(*ws, wavelengths, 4, 4);
    std::array<double, nDetectors> yvalues = {0., 1., 4.};
    for (size_t i = 0; i < indices.size(); i++) {
      auto &ys = sparseWS->mutableY(indices[i]);
      for (size_t j = 0; j < ys.size(); ++j) {
        ys[j] = yvalues[i];
      }
    }

    auto deriv = sparseWS->secondDerivative(indices, 1);
    for (size_t i = 0; i < deriv.size(); ++i) {
      TS_ASSERT_EQUALS(deriv[i], 2.0)
    }
  }

  void test_inverseDistanceWeights() {
    std::array<double, 4> ds{{0.3, 0.3, 0.0, 0.3}};
    auto weights = SparseWorkspaceWrapper::inverseDistanceWeights(ds);
    TS_ASSERT_EQUALS(weights[0], 0.0)
    TS_ASSERT_EQUALS(weights[1], 0.0)
    TS_ASSERT_EQUALS(weights[2], 1.0)
    TS_ASSERT_EQUALS(weights[3], 0.0)
    ds = {{0.2, 0.3, 0.1, 0.4}};
    weights = SparseWorkspaceWrapper::inverseDistanceWeights(ds);
    TS_ASSERT_EQUALS(weights[0], 1 / 0.2 / 0.2)
    TS_ASSERT_EQUALS(weights[1], 1 / 0.3 / 0.3)
    TS_ASSERT_EQUALS(weights[2], 1 / 0.1 / 0.1)
    TS_ASSERT_EQUALS(weights[3], 1 / 0.4 / 0.4)
  }

  void test_efixed_extraction_direct() {
    using namespace WorkspaceCreationHelper;
    auto ws = create2DWorkspaceWithRectangularInstrument(1, 2, 1);
    auto inst = ws->getInstrument();
    auto &pmap = ws->instrumentParameters();
    pmap.addString(inst.get(), "deltaE-mode", "Direct");
    ws->mutableRun().addProperty<double>("Ei", 1.845);
    constexpr size_t gridRows = 3;
    constexpr size_t gridCols = 4;
    auto sparseWS = std::make_unique<SparseWorkspace>(*ws, 1, gridRows, gridCols);
    TS_ASSERT_EQUALS(sparseWS->getEFixed(4), 1.845);
  }

  void test_efixed_extraction_indirect_efixed_on_compassembly() {
    // IRIS stores the efixed on the analyser CompAssembly
    using namespace WorkspaceCreationHelper;
    auto ws = createGroupedWorkspace2D(1, 10, 1.0);
    auto inst = ws->getInstrument();
    auto &pmap = ws->instrumentParameters();
    pmap.addString(inst.get(), "deltaE-mode", "Indirect");
    auto bankComp = inst->getComponentByName("bank1");
    pmap.addDouble(bankComp.get(), "Efixed", 1.845);
    constexpr size_t gridRows = 3;
    constexpr size_t gridCols = 4;
    auto sparseWS = std::make_unique<SparseWorkspace>(*ws, 1, gridRows, gridCols);
    TS_ASSERT_EQUALS(sparseWS->getEFixed(1), 1.845);
  }
};
