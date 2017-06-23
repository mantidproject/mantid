#ifndef MANTID_DATAOBJECTS_SCANNINGWORKSPACEBUILDERTEST_H_
#define MANTID_DATAOBJECTS_SCANNINGWORKSPACEBUILDERTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataObjects/ScanningWorkspaceBuilder.h"

#include "MantidAPI/DetectorInfo.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidHistogramData/BinEdges.h"
#include "MantidHistogramData/Histogram.h"
#include "MantidHistogramData/LinearGenerator.h"
#include "MantidGeometry/Instrument.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidTypes/SpectrumDefinition.h"

#include <cmath>

using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::HistogramData;
using namespace Mantid::Kernel;
using Mantid::DataObjects::ScanningWorkspaceBuilder;

namespace {
Instrument_const_sptr createSimpleInstrument(size_t nDetectors, size_t nBins) {
  const auto &wsWithInstrument =
      WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(
          int(nDetectors), int(nBins));
  return wsWithInstrument->getInstrument();
}
}

class ScanningWorkspaceBuilderTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ScanningWorkspaceBuilderTest *createSuite() {
    return new ScanningWorkspaceBuilderTest();
  }
  static void destroySuite(ScanningWorkspaceBuilderTest *suite) {
    delete suite;
  }

  void tearDown() override {
    positions.clear();
    rotations.clear();
    relativeRotations.clear();
  }

  void test_create_scanning_workspace_with_instrument_and_time_ranges() {
    const auto &instrument = createSimpleInstrument(nDetectors, nBins);

    auto builder = ScanningWorkspaceBuilder(instrument, nTimeIndexes, nBins);
    TS_ASSERT_THROWS_NOTHING(builder.setTimeRanges(timeRanges));
    MatrixWorkspace_const_sptr ws;
    TS_ASSERT_THROWS_NOTHING(ws = builder.buildWorkspace());

    const auto &detectorInfo = ws->detectorInfo();

    // Now check every detector has every time range set correctly
    checkTimeRanges(detectorInfo);
    // Quick check to see if the instrument is set as expected
    TS_ASSERT_EQUALS(instrument->getNumberDetectors(),
                     ws->getInstrument()->getNumberDetectors())
  }

  void test_create_scanning_workspace_with_histogram() {
    const auto &instrument = createSimpleInstrument(nDetectors, nBins);

    BinEdges x(nBins + 1, LinearGenerator(0.0, 1.0));
    Counts y(std::vector<double>(nBins, 5.0));

    auto builder = ScanningWorkspaceBuilder(instrument, nTimeIndexes, nBins);
    TS_ASSERT_THROWS_NOTHING(builder.setTimeRanges(timeRanges));
    TS_ASSERT_THROWS_NOTHING(builder.setHistogram(Histogram(x, y)));
    MatrixWorkspace_const_sptr ws;
    TS_ASSERT_THROWS_NOTHING(ws = builder.buildWorkspace());

    for (size_t i = 0; i < ws->getNumberHistograms(); ++i) {
      const auto &hist = ws->histogram(i);

      const auto &xValues = hist.x();
      for (size_t i = 0; i < xValues.size(); ++i)
        TS_ASSERT_EQUALS(xValues[i], double(i))

      const auto &yValues = hist.y();
      for (size_t i = 0; i < yValues.size(); ++i)
        TS_ASSERT_EQUALS(yValues[i], 5.0)
    }
  }

  void test_create_scanning_workspace_with_incorrectly_sized_histogram() {
    const auto &instrument = createSimpleInstrument(nDetectors, nBins);

    auto wrongNBins = nBins - 2;
    BinEdges x(wrongNBins + 1, LinearGenerator(0.0, 1.0));
    Counts y(std::vector<double>(wrongNBins, 5.0));

    auto builder = ScanningWorkspaceBuilder(instrument, nTimeIndexes, nBins);
    TS_ASSERT_THROWS_EQUALS(
        builder.setHistogram(Histogram(x, y)), const std::logic_error &e,
        std::string(e.what()),
        "Histogram supplied does not have the correct size.")
  }

  void test_create_scanning_workspace_with_time_durations() {
    const auto &instrument = createSimpleInstrument(nDetectors, nBins);

    auto builder = ScanningWorkspaceBuilder(instrument, nTimeIndexes, nBins);
    TS_ASSERT_THROWS_NOTHING(builder.setTimeRanges(0, timeDurations))
    MatrixWorkspace_const_sptr ws;
    TS_ASSERT_THROWS_NOTHING(ws = builder.buildWorkspace())

    const auto &detectorInfo = ws->detectorInfo();

    // Now check every detector has every time range set correctly
    checkTimeRanges(detectorInfo);
  }

  void test_create_scanning_workspace_fails_if_no_time_ranges_set() {
    const auto &instrument = createSimpleInstrument(nDetectors, nBins);

    auto builder = ScanningWorkspaceBuilder(instrument, nTimeIndexes, nBins);

    TS_ASSERT_THROWS_EQUALS(builder.buildWorkspace(), const std::logic_error &e,
                            std::string(e.what()),
                            "Can not build workspace - time ranges have not "
                            "been set. Please call setTimeRanges() before "
                            "building.")
  }

  void
  test_create_scanning_workspace_fails_if_time_ranges_have_the_wrong_dimensions() {
    const auto &instrument = createSimpleInstrument(nDetectors, nBins);

    std::vector<std::pair<DateAndTime, DateAndTime>> timeRangesWrongSize = {
        {0, 1}, {1, 2}};

    auto builder = ScanningWorkspaceBuilder(instrument, nTimeIndexes, nBins);
    TS_ASSERT_THROWS_EQUALS(
        builder.setTimeRanges(std::move(timeRangesWrongSize)),
        const std::logic_error &e, std::string(e.what()),
        "Number of start time, end time pairs supplied "
        "does not match the number of time indexes.")
  }

  void
  test_create_scanning_workspace_fails_if_time_durations_have_the_wrong_dimensions() {
    const auto &instrument = createSimpleInstrument(nDetectors, nBins);

    std::vector<double> timeDurationsWrongSize = {0, 1e-9};

    auto builder = ScanningWorkspaceBuilder(instrument, nTimeIndexes, nBins);
    TS_ASSERT_THROWS_EQUALS(builder.setTimeRanges(0, timeDurationsWrongSize),
                            const std::logic_error &e, std::string(e.what()),
                            "Number of time durations supplied does not match "
                            "the number of time indexes.")
  }

  void test_creating_workspace_with_positions() {
    const auto &instrument = createSimpleInstrument(nDetectors, nBins);

    auto builder = ScanningWorkspaceBuilder(instrument, nTimeIndexes, nBins);
    TS_ASSERT_THROWS_NOTHING(builder.setTimeRanges(timeRanges))
    initalisePositions(nDetectors, nTimeIndexes);
    TS_ASSERT_THROWS_NOTHING(builder.setPositions(std::move(positions)))
    MatrixWorkspace_const_sptr ws;
    TS_ASSERT_THROWS_NOTHING(ws = builder.buildWorkspace())

    const auto &detectorInfo = ws->detectorInfo();

    for (size_t i = 0; i < nDetectors; ++i) {
      for (size_t j = 0; j < nTimeIndexes; ++j) {
        TS_ASSERT_EQUALS(V3D(double(i), double(j), 1.0),
                         detectorInfo.position({i, j}))
      }
    }
  }

  void test_creating_workspace_with_positions_with_too_many_detectors() {
    const auto &instrument = createSimpleInstrument(nDetectors, nBins);

    auto builder = ScanningWorkspaceBuilder(instrument, nTimeIndexes, nBins);
    initalisePositions(nDetectors + 1, nTimeIndexes);
    TS_ASSERT_THROWS_EQUALS(
        builder.setPositions(std::move(positions)), const std::logic_error &e,
        std::string(e.what()),
        "Number of positions supplied does not match the number of detectors.")
  }

  void test_creating_workspace_with_positions_with_too_many_time_indexes() {
    const auto &instrument = createSimpleInstrument(nDetectors, nBins);

    auto builder = ScanningWorkspaceBuilder(instrument, nTimeIndexes, nBins);
    initalisePositions(nDetectors, nTimeIndexes + 1);
    TS_ASSERT_THROWS_EQUALS(builder.setPositions(std::move(positions)),
                            const std::logic_error &e, std::string(e.what()),
                            "Number of positions supplied does not match the "
                            "number of time indexes.")
  }

  void test_creating_workspace_with_rotations() {
    const auto &instrument = createSimpleInstrument(nDetectors, nBins);

    auto builder = ScanningWorkspaceBuilder(instrument, nTimeIndexes, nBins);
    TS_ASSERT_THROWS_NOTHING(builder.setTimeRanges(timeRanges))
    initaliseRotations(nDetectors, nTimeIndexes);
    TS_ASSERT_THROWS_NOTHING(builder.setRotations(std::move(rotations)))
    MatrixWorkspace_const_sptr ws;
    TS_ASSERT_THROWS_NOTHING(ws = builder.buildWorkspace())

    const auto &detectorInfo = ws->detectorInfo();

    for (size_t i = 0; i < nDetectors; ++i) {
      for (size_t j = 0; j < nTimeIndexes; ++j) {
        auto quat = Quat(double(i), double(j), 1.0, 2.0);
        quat.normalize();
        TS_ASSERT_EQUALS(quat, detectorInfo.rotation({i, j}))
      }
    }
  }

  void test_creating_workspace_with_rotations_with_too_many_detectors() {
    const auto &instrument = createSimpleInstrument(nDetectors, nBins);

    auto builder = ScanningWorkspaceBuilder(instrument, nTimeIndexes, nBins);
    initaliseRotations(nDetectors + 1, nTimeIndexes);
    TS_ASSERT_THROWS_EQUALS(
        builder.setRotations(std::move(rotations)), const std::logic_error &e,
        std::string(e.what()),
        "Number of rotations supplied does not match the number of detectors.")
  }

  void test_creating_workspace_with_rotations_with_too_many_time_indexes() {
    const auto &instrument = createSimpleInstrument(nDetectors, nBins);

    auto builder = ScanningWorkspaceBuilder(instrument, nTimeIndexes, nBins);
    initaliseRotations(nDetectors, nTimeIndexes + 1);
    TS_ASSERT_THROWS_EQUALS(builder.setRotations(std::move(rotations)),
                            const std::logic_error &e, std::string(e.what()),
                            "Number of rotations supplied does not match the "
                            "number of time indexes.")
  }

  void test_creating_workspace_with_relative_rotations() {
    const auto &instrument = createSimpleInstrument(nDetectors, nBins);

    auto builder = ScanningWorkspaceBuilder(instrument, nTimeIndexes, nBins);
    TS_ASSERT_THROWS_NOTHING(builder.setTimeRanges(timeRanges))
    initialiseRelativeRotations(nTimeIndexes);
    TS_ASSERT_THROWS_NOTHING(builder.setRelativeRotationsForScans(
        relativeRotations, V3D(0, 0, 0), V3D(0, 1, 0)))
    MatrixWorkspace_const_sptr ws;
    TS_ASSERT_THROWS_NOTHING(ws = builder.buildWorkspace())

    const auto &detInfo = ws->detectorInfo();

    for (size_t i = 0; i < nDetectors; ++i) {
      TS_ASSERT_DELTA(0.0, detInfo.position({i, 0}).X(), 1e-12)
      TS_ASSERT_DELTA(5.0, detInfo.position({i, 0}).Z(), 1e-12)

      TS_ASSERT_DELTA(2.5, detInfo.position({i, 1}).X(), 1e-12)
      TS_ASSERT_DELTA(5.0 * sqrt(3) / 2, detInfo.position({i, 1}).Z(), 1e-12)

      TS_ASSERT_DELTA(5.0 * sqrt(3) / 2, detInfo.position({i, 2}).X(), 1e-12)
      TS_ASSERT_DELTA(2.5, detInfo.position({i, 2}).Z(), 1e-12)

      TS_ASSERT_DELTA(5.0, detInfo.position({i, 3}).X(), 1e-12)
      TS_ASSERT_DELTA(0.0, detInfo.position({i, 3}).Z(), 1e-12)

      for (size_t j = 0; j < nTimeIndexes; ++j) {
        TS_ASSERT_DELTA(double(i) * 0.1, detInfo.position({i, j}).Y(), 1e-12)
      }
    }

    for (size_t i = 0; i < nDetectors; ++i) {
      for (size_t j = 0; j < nTimeIndexes; ++j) {
        // Rounding to nearest int required to avoid problem of Euler angles
        // returning -180/0/180
        TS_ASSERT_DELTA(0.0, std::lround(detInfo.rotation({i, j})
                                             .getEulerAngles("XYZ")[0]) %
                                 180,
                        1e-12)
        TS_ASSERT_DELTA(0.0, std::lround(detInfo.rotation({i, j})
                                             .getEulerAngles("XYZ")[2]) %
                                 180,
                        1e-12)
      }

      TS_ASSERT_DELTA(
          0.0,
          std::lround(detInfo.rotation({i, 0}).getEulerAngles("XYZ")[1]) % 180,
          1e-12)
      TS_ASSERT_DELTA(30.0, detInfo.rotation({i, 1}).getEulerAngles("XYZ")[1],
                      1e-12)
      TS_ASSERT_DELTA(60.0, detInfo.rotation({i, 2}).getEulerAngles("XYZ")[1],
                      1e-12)
      TS_ASSERT_DELTA(90.0, detInfo.rotation({i, 3}).getEulerAngles("XYZ")[1],
                      1e-12)
    }
  }

  void test_creating_workspace_with_relative_rotations_and_offset() {
    const auto &instrument = createSimpleInstrument(nDetectors, nBins);

    auto builder = ScanningWorkspaceBuilder(instrument, nTimeIndexes, nBins);
    TS_ASSERT_THROWS_NOTHING(builder.setTimeRanges(timeRanges))
    initialiseRelativeRotations(nTimeIndexes);
    TS_ASSERT_THROWS_NOTHING(builder.setRelativeRotationsForScans(
        relativeRotations, V3D(0, 0, 1), V3D(0, 1, 0)))
    MatrixWorkspace_const_sptr ws;
    TS_ASSERT_THROWS_NOTHING(ws = builder.buildWorkspace())

    const auto &detInfo = ws->detectorInfo();

    for (size_t i = 0; i < nDetectors; ++i) {
      TS_ASSERT_DELTA(0.0, detInfo.position({i, 0}).X(), 1e-12)
      TS_ASSERT_DELTA(5.0, detInfo.position({i, 0}).Z(), 1e-12)

      TS_ASSERT_DELTA(4.0, detInfo.position({i, 3}).X(), 1e-12)
      TS_ASSERT_DELTA(1.0, detInfo.position({i, 3}).Z(), 1e-12)

      for (size_t j = 0; j < nTimeIndexes; ++j) {
        TS_ASSERT_DELTA(double(i) * 0.1, detInfo.position({i, j}).Y(), 1e-12)
      }
    }
  }

  void
  test_creating_workspace_with_relative_rotations_on_previously_rotated_detectors() {

    const auto &instWS =
        WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(
            int(nDetectors), int(nBins));
    auto &instDetInfo = instWS->mutableDetectorInfo();

    Quat rotation = Quat(90.0, V3D(0, 0, 1));

    for (size_t i = 0; i < instDetInfo.size(); ++i) {
      instDetInfo.setRotation(i, rotation);
    }

    const auto &instrument = instWS->getInstrument();
    TS_ASSERT(instrument->hasDetectorInfo())

    auto builder = ScanningWorkspaceBuilder(instrument, nTimeIndexes, nBins);
    TS_ASSERT_THROWS_NOTHING(builder.setTimeRanges(timeRanges))
    initialiseRelativeRotations(nTimeIndexes);
    TS_ASSERT_THROWS_NOTHING(builder.setRelativeRotationsForScans(
        relativeRotations, V3D(0, 0, 1), V3D(0, 1, 0)))
    MatrixWorkspace_const_sptr ws;
    TS_ASSERT_THROWS_NOTHING(ws = builder.buildWorkspace())

    const auto &detInfo = ws->detectorInfo();

    for (size_t i = 0; i < nDetectors; ++i) {
      for (size_t j = 0; j < nTimeIndexes; ++j) {
        TS_ASSERT_DELTA(0.0, detInfo.rotation({i, j}).getEulerAngles("YXZ")[1],
                        1e-12)
        TS_ASSERT_DELTA(90.0, detInfo.rotation({i, j}).getEulerAngles("YXZ")[2],
                        1e-12)
      }

      TS_ASSERT_DELTA(
          0.0,
          std::lround(detInfo.rotation({i, 0}).getEulerAngles("XYZ")[1]) % 180,
          1e-12)
      TS_ASSERT_DELTA(30.0, detInfo.rotation({i, 1}).getEulerAngles("XYZ")[1],
                      1e-12)
      TS_ASSERT_DELTA(60.0, detInfo.rotation({i, 2}).getEulerAngles("XYZ")[1],
                      1e-12)
      TS_ASSERT_DELTA(90.0, detInfo.rotation({i, 3}).getEulerAngles("XYZ")[1],
                      1e-12)
    }
  }

  void
  test_creating_workspace_with_relative_rotations_fails_with_wrong_time_index_size() {
    const auto &instrument = createSimpleInstrument(nDetectors, nBins);

    auto builder = ScanningWorkspaceBuilder(instrument, nTimeIndexes, nBins);
    initialiseRelativeRotations(nTimeIndexes + 1);
    TS_ASSERT_THROWS_EQUALS(builder.setRelativeRotationsForScans(
                                relativeRotations, V3D(0, 0, 0), V3D(0, 1, 0)),
                            const std::logic_error &e, std::string(e.what()),
                            "Number of instrument angles supplied does not "
                            "match the number of time indexes.")
  }

  void
  test_creating_workspace_with_positions_fails_with_positions_already_set() {
    const auto &instrument = createSimpleInstrument(nDetectors, nBins);

    auto builder = ScanningWorkspaceBuilder(instrument, nTimeIndexes, nBins);
    initalisePositions(nDetectors, nTimeIndexes);
    TS_ASSERT_THROWS_NOTHING(builder.setPositions(std::move(positions)))
    TS_ASSERT_THROWS_EQUALS(builder.setPositions(std::move(positions)),
                            const std::logic_error &e, std::string(e.what()),
                            "Can not set positions, as positions "
                            "or instrument angles have already been set.")
  }

  void
  test_creating_workspace_with_rotations_fails_with_positions_already_set() {
    const auto &instrument = createSimpleInstrument(nDetectors, nBins);

    auto builder = ScanningWorkspaceBuilder(instrument, nTimeIndexes, nBins);
    initaliseRotations(nDetectors, nTimeIndexes);
    TS_ASSERT_THROWS_NOTHING(builder.setRotations(std::move(rotations)))
    TS_ASSERT_THROWS_EQUALS(builder.setRotations(std::move(rotations)),
                            const std::logic_error &e, std::string(e.what()),
                            "Can not set rotations, as rotations "
                            "or instrument angles have already been set.")
  }

  void
  test_creating_workspace_with_positions_fails_with_relative_rotations_set() {
    const auto &instrument = createSimpleInstrument(nDetectors, nBins);

    auto builder = ScanningWorkspaceBuilder(instrument, nTimeIndexes, nBins);
    initialiseRelativeRotations(nTimeIndexes);
    TS_ASSERT_THROWS_NOTHING(builder.setRelativeRotationsForScans(
        relativeRotations, V3D(0, 0, 0), V3D(0, 1, 0)))
    initalisePositions(nDetectors, nTimeIndexes);
    TS_ASSERT_THROWS_EQUALS(builder.setPositions(std::move(positions)),
                            const std::logic_error &e, std::string(e.what()),
                            "Can not set positions, as positions "
                            "or instrument angles have already been set.")
  }

  void
  test_creating_workspace_with_rotations_fails_with_relative_rotations_set() {
    const auto &instrument = createSimpleInstrument(nDetectors, nBins);

    auto builder = ScanningWorkspaceBuilder(instrument, nTimeIndexes, nBins);
    initialiseRelativeRotations(nTimeIndexes);
    TS_ASSERT_THROWS_NOTHING(builder.setRelativeRotationsForScans(
        relativeRotations, V3D(0, 0, 0), V3D(0, 1, 0)))
    initaliseRotations(nDetectors, nTimeIndexes);
    TS_ASSERT_THROWS_EQUALS(builder.setRotations(std::move(rotations)),
                            const std::logic_error &e, std::string(e.what()),
                            "Can not set rotations, as rotations "
                            "or instrument angles have already been set.")
  }

  void
  test_creating_workspace_with_relative_rotations_fails_with_positions_already_set() {
    const auto &instrument = createSimpleInstrument(nDetectors, nBins);

    auto builder = ScanningWorkspaceBuilder(instrument, nTimeIndexes, nBins);
    initalisePositions(nDetectors, nTimeIndexes);
    TS_ASSERT_THROWS_NOTHING(builder.setPositions(std::move(positions)))
    initialiseRelativeRotations(nTimeIndexes);
    TS_ASSERT_THROWS_EQUALS(builder.setRelativeRotationsForScans(
                                relativeRotations, V3D(0, 0, 0), V3D(0, 1, 0)),
                            const std::logic_error &e, std::string(e.what()),
                            "Can not set instrument angles, as positions "
                            "and/or rotations have already been set.")
  }

  void
  test_creating_workspace_with_relative_rotations_fails_with_rotations_already_set() {
    const auto &instrument = createSimpleInstrument(nDetectors, nBins);

    auto builder = ScanningWorkspaceBuilder(instrument, nTimeIndexes, nBins);
    initaliseRotations(nDetectors, nTimeIndexes);
    TS_ASSERT_THROWS_NOTHING(builder.setRotations(std::move(rotations)))
    initialiseRelativeRotations(nTimeIndexes);
    TS_ASSERT_THROWS_EQUALS(builder.setRelativeRotationsForScans(
                                relativeRotations, V3D(0, 0, 0), V3D(0, 1, 0)),
                            const std::logic_error &e, std::string(e.what()),
                            "Can not set instrument angles, as positions "
                            "and/or rotations have already been set.")
  }

  void test_creating_workspace_with_time_oriented_index_info() {
    const auto &instrument = createSimpleInstrument(nDetectors, nBins);

    auto builder = ScanningWorkspaceBuilder(instrument, nTimeIndexes, nBins);
    TS_ASSERT_THROWS_NOTHING(builder.setTimeRanges(timeRanges));
    TS_ASSERT_THROWS_NOTHING(builder.setIndexingType(
        ScanningWorkspaceBuilder::IndexingType::TimeOriented))
    MatrixWorkspace_const_sptr ws;
    TS_ASSERT_THROWS_NOTHING(ws = builder.buildWorkspace());

    const auto &indexInfo = ws->indexInfo();
    const auto &detectorIDs = ws->detectorInfo().detectorIDs();
    const auto &spectrumDefinitions = *(indexInfo.spectrumDefinitions());
    for (size_t i = 0; i < nDetectors; ++i) {
      for (size_t j = 0; j < nTimeIndexes; ++j) {
        const auto index = i * nTimeIndexes + j;
        TS_ASSERT_EQUALS(spectrumDefinitions[index].size(), 1)
        TS_ASSERT_EQUALS(spectrumDefinitions[index][0].first, i)
        TS_ASSERT_EQUALS(spectrumDefinitions[index][0].second, j)
        TS_ASSERT_EQUALS(detectorIDs[spectrumDefinitions[index][0].first],
                         i + 1)
      }
    }
  }

  void test_creating_workspace_with_detector_oriented_index_info() {
    const auto &instrument = createSimpleInstrument(nDetectors, nBins);

    auto builder = ScanningWorkspaceBuilder(instrument, nTimeIndexes, nBins);
    TS_ASSERT_THROWS_NOTHING(builder.setTimeRanges(timeRanges));
    TS_ASSERT_THROWS_NOTHING(builder.setIndexingType(
        ScanningWorkspaceBuilder::IndexingType::DetectorOriented))
    MatrixWorkspace_const_sptr ws;
    TS_ASSERT_THROWS_NOTHING(ws = builder.buildWorkspace());

    const auto &indexInfo = ws->indexInfo();
    const auto &detectorIDs = ws->detectorInfo().detectorIDs();
    const auto &spectrumDefinitions = *(indexInfo.spectrumDefinitions());
    for (size_t i = 0; i < nTimeIndexes; ++i) {
      for (size_t j = 0; j < nDetectors; ++j) {
        const auto index = i * nDetectors + j;
        TS_ASSERT_EQUALS(spectrumDefinitions[index].size(), 1)
        TS_ASSERT_EQUALS(spectrumDefinitions[index][0].first, j)
        TS_ASSERT_EQUALS(spectrumDefinitions[index][0].second, i)
        TS_ASSERT_EQUALS(detectorIDs[spectrumDefinitions[index][0].first],
                         j + 1)
      }
    }
  }

  void test_setting_indexing_type_twice_throws_and_error() {
    const auto &instrument = createSimpleInstrument(nDetectors, nBins);

    auto builder = ScanningWorkspaceBuilder(instrument, nTimeIndexes, nBins);
    TS_ASSERT_THROWS_NOTHING(builder.setTimeRanges(timeRanges));
    TS_ASSERT_THROWS_NOTHING(builder.setIndexingType(
        ScanningWorkspaceBuilder::IndexingType::DetectorOriented))
    TS_ASSERT_THROWS_EQUALS(
        builder.setIndexingType(
            ScanningWorkspaceBuilder::IndexingType::TimeOriented),
        const std::logic_error &e, std::string(e.what()),
        "Indexing type has been set already.")
  }

private:
  size_t nDetectors = 5;
  size_t nTimeIndexes = 4;
  size_t nBins = 10;

  const std::vector<std::pair<DateAndTime, DateAndTime>> timeRanges = {
      {0, 2}, {2, 3}, {3, 6}, {6, 10}};

  std::vector<double> timeDurations = {2e-9, 1e-9, 3e-9, 4e-9};

  std::vector<std::vector<V3D>> positions;
  std::vector<std::vector<Quat>> rotations;
  std::vector<double> relativeRotations;

  void initalisePositions(size_t nDetectors, size_t nTimeIndexes) {
    for (size_t i = 0; i < nDetectors; ++i) {
      std::vector<V3D> timePositions;
      for (size_t j = 0; j < nTimeIndexes; ++j) {
        timePositions.push_back(V3D(double(i), double(j), 1.0));
      }
      positions.push_back(timePositions);
    }
  }

  void initaliseRotations(size_t nDetectors, size_t nTimeIndexes) {
    for (size_t i = 0; i < nDetectors; ++i) {
      std::vector<Quat> timeRotations;
      for (size_t j = 0; j < nTimeIndexes; ++j) {
        timeRotations.push_back(Quat(double(i), double(j), 1.0, 2.0));
      }
      rotations.push_back(timeRotations);
    }
  }

  void initialiseRelativeRotations(size_t nTimeIndexes) {
    for (size_t i = 0; i < nTimeIndexes; ++i) {
      relativeRotations.push_back(double(i) * 30.0);
    }
  }

  Instrument_const_sptr createSimpleInstrument(size_t nDetectors,
                                               size_t nBins) {
    const auto &wsWithInstrument =
        WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(
            int(nDetectors), int(nBins));
    return wsWithInstrument->getInstrument();
  }

  void checkTimeRanges(const DetectorInfo &detectorInfo) {
    for (size_t i = 0; i < nDetectors; ++i) {
      for (size_t j = 0; j < nTimeIndexes; ++j) {
        TS_ASSERT_EQUALS(detectorInfo.scanInterval({i, j}), timeRanges[j]);
      }
    }
  }
};

class ScanningWorkspaceBuilderTestPerformance : public CxxTest::TestSuite {
public:
  void test_large_scanning_workspace() {
    make_scanning_workspace(1000, 500, 1000);
  }

  void test_lots_of_small_scanning_workspaces() {
    for (size_t i = 0; i < 200; ++i)
      make_scanning_workspace(100, 50, 100);
  }

  void make_scanning_workspace(size_t nDetectors, size_t nTimeIndexes,
                               size_t nBins) {

    const auto &instrument = createSimpleInstrument(nDetectors, nTimeIndexes);

    std::vector<std::pair<DateAndTime, DateAndTime>> timeRanges;
    for (size_t i = 0; i < nTimeIndexes; ++i) {
      timeRanges.push_back(std::pair<DateAndTime, DateAndTime>(
          DateAndTime(i * 2), DateAndTime(i * 2 + 1)));
    }

    auto builder = ScanningWorkspaceBuilder(instrument, nTimeIndexes, nBins);
    builder.setTimeRanges(timeRanges);
    MatrixWorkspace_const_sptr ws;
    ws = builder.buildWorkspace();
  }
};

#endif /* MANTID_DATAOBJECTS_SCANNINGWORKSPACEBUILDERTEST_H_ */
