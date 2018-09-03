#ifndef SPARSEINSTRUMENTTEST_H_
#define SPARSEINSTRUMENTTEST_H_

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

using namespace Mantid::Algorithms::SparseInstrument;
using namespace Mantid::Geometry;

class SparseInstrumentTest : public CxxTest::TestSuite {
public:
  static SparseInstrumentTest *createSuite() {
    return new SparseInstrumentTest();
  }

  static void destroySuite(SparseInstrumentTest *suite) { delete suite; }

  SparseInstrumentTest()
      : m_goofyRefFrame(PointingAlong::X, PointingAlong::Y, Handedness::Left,
                        ""),
        m_standardRefFrame(PointingAlong::Y, PointingAlong::Z,
                           Handedness::Right, "") {}

  void test_createSparseWS() {
    using namespace WorkspaceCreationHelper;
    auto ws = create2DWorkspaceWithRectangularInstrument(1, 2, 10);
    const size_t gridRows = 5;
    const size_t gridCols = 3;
    const auto grid = createDetectorGridDefinition(*ws, gridRows, gridCols);
    const size_t wavelengths = 3;
    auto sparseWS = createSparseWS(*ws, *grid, wavelengths);
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
    std::tie(minLat, maxLat, minLon, maxLon) = extremeAngles(*ws);
    double sparseMinLat;
    double sparseMaxLat;
    double sparseMinLon;
    double sparseMaxLon;
    std::tie(sparseMinLat, sparseMaxLat, sparseMinLon, sparseMaxLon) =
        extremeAngles(*sparseWS);
    TS_ASSERT_EQUALS(sparseMinLat, minLat)
    TS_ASSERT_DELTA(sparseMaxLat, maxLat, 1e-8)
    TS_ASSERT_EQUALS(sparseMinLon, minLon)
    TS_ASSERT_DELTA(sparseMaxLon, maxLon, 1e-8)
  }

  void test_extremeAngles_multipleDetectors() {
    using namespace WorkspaceCreationHelper;
    auto ws = create2DWorkspaceWithRectangularInstrument(1, 2, 1);
    const auto &spectrumInfo = ws->spectrumInfo();
    auto refFrame = ws->getInstrument()->getReferenceFrame();
    double minLat;
    double minLon;
    double maxLat;
    double maxLon;
    std::tie(minLat, maxLat, minLon, maxLon) = extremeAngles(*ws);
    for (size_t i = 0; i < ws->getNumberHistograms(); ++i) {
      const auto pos = spectrumInfo.position(i);
      double lat;
      double lon;
      std::tie(lat, lon) = geographicalAngles(pos, *refFrame);
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
    std::tie(minLat, maxLat, minLon, maxLon) = extremeAngles(*ws);
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
    std::tie(minWavelength, maxWavelength) = extremeWavelengths(*ws);
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
    std::tie(minWavelength, maxWavelength) = extremeWavelengths(*ws);
    TS_ASSERT_EQUALS(minWavelength, -3.0)
    TS_ASSERT_EQUALS(maxWavelength, 4.0)
  }

  void test_geographicalAngles_casualAngles() {
    using Mantid::Kernel::V3D;
    V3D v;
    v[m_standardRefFrame.pointingHorizontal()] = 1.0;
    v[m_standardRefFrame.pointingUp()] = 1.0;
    double lat, lon;
    std::tie(lat, lon) = geographicalAngles(v, m_standardRefFrame);
    TS_ASSERT_EQUALS(lat, M_PI / 4);
    TS_ASSERT_EQUALS(lon, M_PI / 2);
    v *= 0;
    v[m_goofyRefFrame.pointingHorizontal()] = 1.0;
    v[m_goofyRefFrame.pointingUp()] = 1.0;
    std::tie(lat, lon) = geographicalAngles(v, m_goofyRefFrame);
    TS_ASSERT_EQUALS(lat, M_PI / 4);
    TS_ASSERT_EQUALS(lon, M_PI / 2);
  }

  void test_geographicalAngles_poles() {
    using Mantid::Kernel::V3D;
    V3D v = m_standardRefFrame.vecPointingUp();
    double lat, lon;
    std::tie(lat, lon) = geographicalAngles(v, m_standardRefFrame);
    TS_ASSERT_EQUALS(lat, M_PI / 2);
    TS_ASSERT_EQUALS(lon, 0.0);
    v *= -1;
    std::tie(lat, lon) = geographicalAngles(v, m_standardRefFrame);
    TS_ASSERT_EQUALS(lat, -M_PI / 2);
    TS_ASSERT_EQUALS(lon, -M_PI);
    v = m_goofyRefFrame.vecPointingUp();
    std::tie(lat, lon) = geographicalAngles(v, m_goofyRefFrame);
    TS_ASSERT_EQUALS(lat, M_PI / 2);
    TS_ASSERT_EQUALS(lon, 0.0);
    v *= -1;
    std::tie(lat, lon) = geographicalAngles(v, m_goofyRefFrame);
    TS_ASSERT_EQUALS(lat, -M_PI / 2);
    TS_ASSERT_EQUALS(lon, -M_PI);
  }

  void test_geographicalAngles_zeroAngles() {
    using Mantid::Kernel::V3D;
    V3D v = m_standardRefFrame.vecPointingAlongBeam();
    double lat, lon;
    std::tie(lat, lon) = geographicalAngles(v, m_standardRefFrame);
    TS_ASSERT_EQUALS(lat, 0.0);
    TS_ASSERT_EQUALS(lon, 0.0);
    v = m_goofyRefFrame.vecPointingAlongBeam();
    std::tie(lat, lon) = geographicalAngles(v, m_goofyRefFrame);
    TS_ASSERT_EQUALS(lat, 0.0);
    TS_ASSERT_EQUALS(lon, 0.0);
  }

  void test_greatCircleDistance() {
    double d = greatCircleDistance(0, 0, 0, 0);
    TS_ASSERT_EQUALS(d, 0.0)
    d = greatCircleDistance(M_PI / 2, 0.0, -M_PI / 2, 0.0);
    TS_ASSERT_EQUALS(d, M_PI)
    d = greatCircleDistance(M_PI / 4, M_PI / 4, -M_PI / 4, -M_PI / 4);
    TS_ASSERT_DELTA(d, 2 * M_PI / 3, 1e-8)
  }

  void test_interpolateFromDetectorGrid() {
    using namespace WorkspaceCreationHelper;
    auto ws = create2DWorkspaceWithRectangularInstrument(1, 2, 7);
    const size_t sparseRows = 3;
    const size_t sparseCols = 6;
    auto grid = createDetectorGridDefinition(*ws, sparseRows, sparseCols);
    const size_t wavelengths = 3;
    auto sparseWS = createSparseWS(*ws, *grid, wavelengths).release();
    for (size_t i = 0; i < sparseWS->getNumberHistograms(); ++i) {
      auto &ys = sparseWS->mutableY(i);
      auto &es = sparseWS->mutableE(i);
      for (size_t j = 0; j < ys.size(); ++j) {
        ys[j] = static_cast<double>(i);
        es[j] = std::sqrt(ys[j]);
      }
    }
    double lat = grid->latitudeAt(0);
    double lon = grid->longitudeAt(0);
    auto indices = grid->nearestNeighbourIndices(lat, lon);
    auto h = interpolateFromDetectorGrid(lat, lon, *sparseWS, indices);
    TS_ASSERT_EQUALS(h.size(), wavelengths)
    for (size_t i = 0; i < h.size(); ++i) {
      TS_ASSERT_EQUALS(h.y()[i], 0.0)
      TS_ASSERT_EQUALS(h.e()[i], 0.0)
    }
    lat = (grid->latitudeAt(2) + grid->latitudeAt(1)) / 2.0;
    lon = (grid->longitudeAt(3) + grid->longitudeAt(2)) / 2.0;
    indices = grid->nearestNeighbourIndices(lat, lon);
    double val =
        static_cast<double>(indices[0] + indices[1] + indices[2] + indices[3]) /
        4.0;
    h = interpolateFromDetectorGrid(lat, lon, *sparseWS, indices);
    TS_ASSERT_EQUALS(h.size(), wavelengths)
    for (size_t i = 0; i < h.size(); ++i) {
      TS_ASSERT_DELTA(h.y()[i], val, 1e-7)
      TS_ASSERT_EQUALS(h.e()[i], 0.0)
    }
  }

  void test_inverseDistanceWeights() {
    std::array<double, 4> ds{{0.3, 0.3, 0.0, 0.3}};
    auto weights = inverseDistanceWeights(ds);
    TS_ASSERT_EQUALS(weights[0], 0.0)
    TS_ASSERT_EQUALS(weights[1], 0.0)
    TS_ASSERT_EQUALS(weights[2], 1.0)
    TS_ASSERT_EQUALS(weights[3], 0.0)
    ds = {{0.2, 0.3, 0.1, 0.4}};
    weights = inverseDistanceWeights(ds);
    TS_ASSERT_EQUALS(weights[0], 1 / 0.2 / 0.2)
    TS_ASSERT_EQUALS(weights[1], 1 / 0.3 / 0.3)
    TS_ASSERT_EQUALS(weights[2], 1 / 0.1 / 0.1)
    TS_ASSERT_EQUALS(weights[3], 1 / 0.4 / 0.4)
  }

  void test_createDetectorGridDefinition_multipleDetectors() {
    using namespace WorkspaceCreationHelper;
    auto ws = create2DWorkspaceWithRectangularInstrument(1, 2, 1);
    const size_t gridRows = 3;
    const size_t gridCols = 4;
    auto grid = createDetectorGridDefinition(*ws, gridRows, gridCols);
    TS_ASSERT_EQUALS(grid->numberRows(), gridRows)
    TS_ASSERT_EQUALS(grid->numberColumns(), gridCols)
    const auto &spectrumInfo = ws->spectrumInfo();
    auto pos = spectrumInfo.position(0);
    auto refFrame = ws->getInstrument()->getReferenceFrame();
    double lat;
    double lon;
    std::tie(lat, lon) = geographicalAngles(pos, *refFrame);
    TS_ASSERT_EQUALS(grid->latitudeAt(0), lat)
    TS_ASSERT_EQUALS(grid->longitudeAt(0), lon)
    pos = spectrumInfo.position(3);
    std::tie(lat, lon) = geographicalAngles(pos, *refFrame);
    TS_ASSERT_DELTA(grid->latitudeAt(gridRows - 1), lat, 1e-8)
    TS_ASSERT_EQUALS(grid->longitudeAt(gridCols - 1), lon)
  }

  void test_createDetectorGridDefinition_singleDetector() {
    using namespace WorkspaceCreationHelper;
    auto ws = create2DWorkspaceWithRectangularInstrument(1, 1, 1);
    const auto pos = ws->spectrumInfo().position(0);
    auto refFrame = ws->getInstrument()->getReferenceFrame();
    double lat;
    double lon;
    std::tie(lat, lon) = geographicalAngles(pos, *refFrame);
    auto grid = createDetectorGridDefinition(*ws, 2, 2);
    TS_ASSERT_EQUALS(grid->numberColumns(), 2)
    TS_ASSERT_EQUALS(grid->numberRows(), 2)
    TS_ASSERT_LESS_THAN(grid->latitudeAt(0), lat)
    TS_ASSERT_LESS_THAN(grid->longitudeAt(0), lon)
    TS_ASSERT_LESS_THAN(lat, grid->latitudeAt(1))
    TS_ASSERT_LESS_THAN(lon, grid->longitudeAt(1))
  }

  void test_modelHistogram_coversModelWS() {
    using namespace Mantid::DataObjects;
    using namespace Mantid::HistogramData;
    const BinEdges edges(256, LinearGenerator(-1.33, 0.77));
    const Counts counts(edges.size() - 1, 0.);
    auto ws = create<Workspace2D>(2, Histogram(edges, counts));
    const auto points = ws->points(0);
    for (size_t nCounts = 2; nCounts < counts.size(); ++nCounts) {
      const auto histo = modelHistogram(*ws, nCounts);
      // These have to be equal, don't use DELTA here!
      TS_ASSERT_EQUALS(histo.x().front(), points.front())
      TS_ASSERT_EQUALS(histo.x().back(), points.back())
    }
  }

private:
  const Mantid::Geometry::ReferenceFrame m_goofyRefFrame;
  const Mantid::Geometry::ReferenceFrame m_standardRefFrame;
};

#endif // SPARSEINSTRUMENTTEST_H_
