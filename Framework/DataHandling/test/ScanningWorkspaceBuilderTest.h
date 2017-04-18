#ifndef MANTID_DATAHANDLING_SCANNINGWORKSPACEBUILDERTEST_H_
#define MANTID_DATAHANDLING_SCANNINGWORKSPACEBUILDERTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/ScanningWorkspaceBuilder.h"

#include "MantidAPI/DetectorInfo.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidTypes/SpectrumDefinition.h"

#include <cmath>

using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;
using Mantid::DataHandling::ScanningWorkspaceBuilder;

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
    instrumentAngles.clear();
  }

  void test_create_scanning_workspace_with_too_small_instrument() {
    const auto &instrument = createSimpleInstrument(nDetectors, nBins);

    auto builder =
        ScanningWorkspaceBuilder(nDetectors + 1, nTimeIndexes, nBins);
    TS_ASSERT_THROWS_EQUALS(builder.setInstrument(instrument),
                            const std::logic_error &e, std::string(e.what()),
                            "There are not enough detectors in the instrument "
                            "for the number of detectors set in the scanning "
                            "workspace builder.")
  }

  void test_create_scanning_workspace_with_correct_time_ranges() {
    const auto &instrument = createSimpleInstrument(nDetectors, nBins);

    auto builder = ScanningWorkspaceBuilder(nDetectors, nTimeIndexes, nBins);
    TS_ASSERT_THROWS_NOTHING(builder.setInstrument(instrument));
    TS_ASSERT_THROWS_NOTHING(builder.setTimeRanges(timeRanges));
    MatrixWorkspace_const_sptr ws;
    TS_ASSERT_THROWS_NOTHING(ws = builder.buildWorkspace());

    const auto &detectorInfo = ws->detectorInfo();

    // Now check every detector has every time range set correctly
    checkTimeRanges(detectorInfo);
  }

  void test_create_scanning_workspace_with_correct_time_durations() {
    const auto &instrument = createSimpleInstrument(nDetectors, nBins);

    auto builder = ScanningWorkspaceBuilder(nDetectors, nTimeIndexes, nBins);
    TS_ASSERT_THROWS_NOTHING(builder.setInstrument(instrument))
    TS_ASSERT_THROWS_NOTHING(builder.setTimeRanges(0, timeDurations))
    MatrixWorkspace_const_sptr ws;
    TS_ASSERT_THROWS_NOTHING(ws = builder.buildWorkspace())

    const auto &detectorInfo = ws->detectorInfo();

    // Now check every detector has every time range set correctly
    checkTimeRanges(detectorInfo);
  }

  void test_create_scanning_workspace_fails_if_no_instrument_set() {
    auto builder = ScanningWorkspaceBuilder(nDetectors, nTimeIndexes, nBins);
    TS_ASSERT_THROWS_EQUALS(builder.buildWorkspace(), const std::logic_error &e,
                            std::string(e.what()),
                            "Can not build workspace - instrument has not been "
                            "set. Please call setInstrument() before "
                            "building.")
  }

  void test_create_scanning_workspace_fails_if_no_time_ranges_set() {
    const auto &instrument = createSimpleInstrument(nDetectors, nBins);

    auto builder = ScanningWorkspaceBuilder(nDetectors, nTimeIndexes, nBins);
    TS_ASSERT_THROWS_NOTHING(builder.setInstrument(instrument))

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

    auto builder = ScanningWorkspaceBuilder(nDetectors, nTimeIndexes, nBins);
    TS_ASSERT_THROWS_NOTHING(builder.setInstrument(instrument))
    TS_ASSERT_THROWS_EQUALS(builder.setTimeRanges(timeRangesWrongSize),
                            const std::logic_error &e, std::string(e.what()),
                            "Number of start time, end time pairs supplied "
                            "does not match the number of time indexes.")
  }

  void
  test_create_scanning_workspace_fails_if_time_durations_have_the_wrong_dimensions() {
    const auto &instrument = createSimpleInstrument(nDetectors, nBins);

    std::vector<double> timeDurationsWrongSize = {0, 1e-9};

    auto builder = ScanningWorkspaceBuilder(nDetectors, nTimeIndexes, nBins);
    TS_ASSERT_THROWS_NOTHING(builder.setInstrument(instrument))
    TS_ASSERT_THROWS_EQUALS(builder.setTimeRanges(0, timeDurationsWrongSize),
                            const std::logic_error &e, std::string(e.what()),
                            "Number of time durations supplied does not match "
                            "the number of time indexes.")
  }

  void test_creating_workspace_with_positions() {
    const auto &instrument = createSimpleInstrument(nDetectors, nBins);

    auto builder = ScanningWorkspaceBuilder(nDetectors, nTimeIndexes, nBins);
    TS_ASSERT_THROWS_NOTHING(builder.setInstrument(instrument))
    TS_ASSERT_THROWS_NOTHING(builder.setTimeRanges(timeRanges))
    initalisePositions(nDetectors, nTimeIndexes);
    TS_ASSERT_THROWS_NOTHING(builder.setPositions(positions))
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
    auto builder = ScanningWorkspaceBuilder(nDetectors, nTimeIndexes, nBins);
    initalisePositions(nDetectors + 1, nTimeIndexes);
    TS_ASSERT_THROWS_EQUALS(
        builder.setPositions(positions), const std::logic_error &e,
        std::string(e.what()),
        "Number of positions supplied does not match the number of detectors.")
  }

  void test_creating_workspace_with_positions_with_too_many_time_indexes() {
    auto builder = ScanningWorkspaceBuilder(nDetectors, nTimeIndexes, nBins);
    initalisePositions(nDetectors, nTimeIndexes + 1);
    TS_ASSERT_THROWS_EQUALS(builder.setPositions(positions),
                            const std::logic_error &e, std::string(e.what()),
                            "Number of positions supplied does not match the "
                            "number of time indexes.")
  }

  void test_creating_workspace_with_rotations() {
    const auto &instrument = createSimpleInstrument(nDetectors, nBins);

    auto builder = ScanningWorkspaceBuilder(nDetectors, nTimeIndexes, nBins);
    TS_ASSERT_THROWS_NOTHING(builder.setInstrument(instrument))
    TS_ASSERT_THROWS_NOTHING(builder.setTimeRanges(timeRanges))
    initaliseRotations(nDetectors, nTimeIndexes);
    TS_ASSERT_THROWS_NOTHING(builder.setRotations(rotations))
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
    auto builder = ScanningWorkspaceBuilder(nDetectors, nTimeIndexes, nBins);
    initaliseRotations(nDetectors + 1, nTimeIndexes);
    TS_ASSERT_THROWS_EQUALS(
        builder.setRotations(rotations), const std::logic_error &e,
        std::string(e.what()),
        "Number of rotations supplied does not match the number of detectors.")
  }

  void test_creating_workspace_with_rotations_with_too_many_time_indexes() {
    auto builder = ScanningWorkspaceBuilder(nDetectors, nTimeIndexes, nBins);
    initaliseRotations(nDetectors, nTimeIndexes + 1);
    TS_ASSERT_THROWS_EQUALS(builder.setRotations(rotations),
                            const std::logic_error &e, std::string(e.what()),
                            "Number of rotations supplied does not match the "
                            "number of time indexes.")
  }

  void test_creating_workspace_with_instrument_angles() {
    const auto &instrument = createSimpleInstrument(nDetectors, nBins);

    auto builder = ScanningWorkspaceBuilder(nDetectors, nTimeIndexes, nBins);
    TS_ASSERT_THROWS_NOTHING(builder.setInstrument(instrument))
    TS_ASSERT_THROWS_NOTHING(builder.setTimeRanges(timeRanges))
    initialiseInstrumentAngles(nTimeIndexes);
    TS_ASSERT_THROWS_NOTHING(builder.setInstrumentAngles(instrumentAngles))
    MatrixWorkspace_const_sptr ws;
    TS_ASSERT_THROWS_NOTHING(ws = builder.buildWorkspace())

    const auto &detectorInfo = ws->detectorInfo();

    for (size_t i = 0; i < nDetectors; ++i) {
      TS_ASSERT_DELTA(0.0, detectorInfo.position({i, 0}).X(), 1e-12)
      TS_ASSERT_DELTA(5.0, detectorInfo.position({i, 0}).Z(), 1e-12)

      TS_ASSERT_DELTA(2.5, detectorInfo.position({i, 1}).X(), 1e-12)
      TS_ASSERT_DELTA(5.0 * sqrt(3) / 2, detectorInfo.position({i, 1}).Z(),
                      1e-12)

      TS_ASSERT_DELTA(5.0 * sqrt(3) / 2, detectorInfo.position({i, 2}).X(),
                      1e-12)
      TS_ASSERT_DELTA(2.5, detectorInfo.position({i, 2}).Z(), 1e-12)

      TS_ASSERT_DELTA(5.0, detectorInfo.position({i, 3}).X(), 1e-12)
      TS_ASSERT_DELTA(0.0, detectorInfo.position({i, 3}).Z(), 1e-12)

      for (size_t j = 0; j < nTimeIndexes; ++j) {
        TS_ASSERT_EQUALS(0.0, detectorInfo.position({double(i) * 0.1, j}).Y())
      }
    }

    for (size_t i = 0; i < nDetectors; ++i) {
      for (size_t j = 0; j < nTimeIndexes; ++j) {
        TS_ASSERT_DELTA(
            0.0, detectorInfo.rotation({i, j}).getEulerAngles("XYZ")[0], 1e-12)
        TS_ASSERT_DELTA(
            0.0, detectorInfo.rotation({i, j}).getEulerAngles("XYZ")[2], 1e-12)
      }

      TS_ASSERT_DELTA(
          0.0, detectorInfo.rotation({i, 0}).getEulerAngles("XYZ")[1], 1e-12)
      TS_ASSERT_DELTA(
          30.0, detectorInfo.rotation({i, 1}).getEulerAngles("XYZ")[1], 1e-12)
      TS_ASSERT_DELTA(
          60.0, detectorInfo.rotation({i, 2}).getEulerAngles("XYZ")[1], 1e-12)
      TS_ASSERT_DELTA(
          90.0, detectorInfo.rotation({i, 3}).getEulerAngles("XYZ")[1], 1e-12)
    }
  }

  void
  test_creating_workspace_with_instrument_angles_fails_with_wrong_time_index_size() {
    auto builder = ScanningWorkspaceBuilder(nDetectors, nTimeIndexes, nBins);
    initialiseInstrumentAngles(nTimeIndexes + 1);
    TS_ASSERT_THROWS_EQUALS(builder.setInstrumentAngles(instrumentAngles),
                            const std::logic_error &e, std::string(e.what()),
                            "Number of instrument angles supplied does not "
                            "match the number of time indexes.")
  }

  void
  test_creating_workspace_with_instrument_angles_fails_with_positions_already_set() {
    auto builder = ScanningWorkspaceBuilder(nDetectors, nTimeIndexes, nBins);
    initalisePositions(nDetectors, nTimeIndexes);
    TS_ASSERT_THROWS_NOTHING(builder.setPositions(positions))
    initialiseInstrumentAngles(nTimeIndexes);
    TS_ASSERT_THROWS_EQUALS(builder.setInstrumentAngles(instrumentAngles),
                            const std::logic_error &e, std::string(e.what()),
                            "Can not set instrument angles, as positions "
                            "and/or rotations have already been set.")
  }

  void test_creating_workspace_with_time_oriented_index_info() {
    const auto &instrument = createSimpleInstrument(nDetectors, nBins);

    auto builder = ScanningWorkspaceBuilder(nDetectors, nTimeIndexes, nBins);
    TS_ASSERT_THROWS_NOTHING(builder.setInstrument(instrument));
    TS_ASSERT_THROWS_NOTHING(builder.setTimeRanges(timeRanges));
    TS_ASSERT_THROWS_NOTHING(builder.setIndexingType(
        ScanningWorkspaceBuilder::IndexingType::TIME_ORIENTED))
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

    auto builder = ScanningWorkspaceBuilder(nDetectors, nTimeIndexes, nBins);
    TS_ASSERT_THROWS_NOTHING(builder.setInstrument(instrument));
    TS_ASSERT_THROWS_NOTHING(builder.setTimeRanges(timeRanges));
    TS_ASSERT_THROWS_NOTHING(builder.setIndexingType(
        ScanningWorkspaceBuilder::IndexingType::DETECTOR_ORIENTED))
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

    auto builder = ScanningWorkspaceBuilder(nDetectors, nTimeIndexes, nBins);
    TS_ASSERT_THROWS_NOTHING(builder.setInstrument(instrument));
    TS_ASSERT_THROWS_NOTHING(builder.setTimeRanges(timeRanges));
    TS_ASSERT_THROWS_NOTHING(builder.setIndexingType(
        ScanningWorkspaceBuilder::IndexingType::DETECTOR_ORIENTED))
    TS_ASSERT_THROWS_EQUALS(
        builder.setIndexingType(
            ScanningWorkspaceBuilder::IndexingType::TIME_ORIENTED),
        const std::logic_error &e, std::string(e.what()),
        "Indexing type has been set already.")
  }

private:
  size_t nDetectors = 5;
  size_t nTimeIndexes = 4;
  size_t nBins = 10;

  std::vector<std::pair<DateAndTime, DateAndTime>> timeRanges = {
      {0, 1}, {1, 3}, {3, 6}, {6, 10}};

  std::vector<double> timeDurations = {1e-9, 2e-9, 3e-9, 4e-9};

  std::vector<std::vector<V3D>> positions;
  std::vector<std::vector<Quat>> rotations;
  std::vector<double> instrumentAngles;

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

  void initialiseInstrumentAngles(size_t nTimeIndexes) {
    for (size_t i = 0; i < nTimeIndexes; ++i) {
      instrumentAngles.push_back(double(i) * 30.0);
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

#endif /* MANTID_DATAHANDLING_SCANNINGWORKSPACEBUILDERTEST_H_ */
