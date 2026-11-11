// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/ISpectrum.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataHandling/InstrumentSpectraMapping.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidKernel/Logger.h"

#include <memory>
#include <set>
#include <string>
#include <vector>

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::DataHandling;
using Mantid::Geometry::Detector;
using Mantid::Geometry::Instrument;

class InstrumentSpectraMappingTest : public CxxTest::TestSuite {
public:
  static InstrumentSpectraMappingTest *createSuite() { return new InstrumentSpectraMappingTest(); }
  static void destroySuite(InstrumentSpectraMappingTest *suite) { delete suite; }

  // ---------------------------------------------------------------------------------------------------------------
  // The instrument has to enable the correction before anything happens
  // ---------------------------------------------------------------------------------------------------------------

  void test_an_instrument_that_does_not_enable_the_correction_is_left_alone() {
    auto workspace = createWorkspace(2, {1, 2}, {{9001}, {9002}}, "");

    TS_ASSERT(!correctSpectraMapping(*workspace, m_log));
    assertDetectorIDs(*workspace, 0, {9001});
    assertDetectorIDs(*workspace, 1, {9002});
  }

  void test_an_instrument_enabling_some_other_source_is_left_alone() {
    auto workspace = createWorkspace(2, {1, 2}, {{9001}, {9002}}, "datafile");

    TS_ASSERT(!correctSpectraMapping(*workspace, m_log));
    assertDetectorIDs(*workspace, 0, {9001});
  }

  void test_a_workspace_whose_instrument_defines_no_detectors_is_left_alone() {
    auto workspace = createWorkspace(2, {}, {{9001}, {9002}});

    TS_ASSERT(!correctSpectraMapping(*workspace, m_log));
    assertDetectorIDs(*workspace, 0, {9001});
  }

  // ---------------------------------------------------------------------------------------------------------------
  // The correction
  // ---------------------------------------------------------------------------------------------------------------

  void test_a_table_referencing_only_known_detectors_is_left_alone() {
    // The detectors are all defined by the instrument, even though they are not the ones an identity mapping picks.
    auto workspace = createWorkspace(3, {1, 2, 3}, {{3}, {1}, {2}});

    TS_ASSERT(!correctSpectraMapping(*workspace, m_log));
    assertDetectorIDs(*workspace, 0, {3});
    assertDetectorIDs(*workspace, 1, {1});
    assertDetectorIDs(*workspace, 2, {2});
  }

  void test_every_spectrum_is_mapped_to_the_detector_of_the_same_id_when_the_table_is_unusable() {
    auto workspace = createWorkspace(3, {1, 2, 3}, {{9001}, {9002}, {9003}});

    TS_ASSERT(correctSpectraMapping(*workspace, m_log));
    assertDetectorIDs(*workspace, 0, {1});
    assertDetectorIDs(*workspace, 1, {2});
    assertDetectorIDs(*workspace, 2, {3});
  }

  void test_a_spectrum_mapped_to_several_hardware_detectors_is_collapsed_onto_one() {
    auto workspace = createWorkspace(2, {1, 2}, {{9001, 9002, 9003}, {9004, 9005, 9006}});

    TS_ASSERT(correctSpectraMapping(*workspace, m_log));
    assertDetectorIDs(*workspace, 0, {1});
    assertDetectorIDs(*workspace, 1, {2});
  }

  void test_only_the_spectra_that_need_correcting_are_touched() {
    // Spectrum 1 is deliberately mapped to a detector that is not its own number, and spectrum 2 to two of them.
    // Both are detectors the instrument defines, so both mappings are ones the file got right and must survive the
    // correction of spectrum 3.
    auto workspace = createWorkspace(3, {1, 2, 3}, {{3}, {1, 2}, {9003}});

    TS_ASSERT(correctSpectraMapping(*workspace, m_log));
    assertDetectorIDs(*workspace, 0, {3});
    assertDetectorIDs(*workspace, 1, {1, 2});
    assertDetectorIDs(*workspace, 2, {3});
  }

  void test_a_spectrum_the_instrument_has_no_detector_for_keeps_its_histogram_without_detectors() {
    // The spare hardware channel a RAW file records after the last pixel: spectrum 3 has no detector 3 to map to.
    auto workspace = createWorkspace(3, {1, 2}, {{9001}, {9002}, {9003, 9004}});
    auto const countsBefore = workspace->y(2).rawData();

    TS_ASSERT(correctSpectraMapping(*workspace, m_log));
    assertDetectorIDs(*workspace, 0, {1});
    assertDetectorIDs(*workspace, 1, {2});
    TS_ASSERT(workspace->getSpectrum(2).getDetectorIDs().empty());
    TS_ASSERT_EQUALS(countsBefore, workspace->y(2).rawData());
  }

  void test_a_spectrum_with_no_detectors_at_all_is_left_without_any() {
    auto workspace = createWorkspace(2, {1, 2}, {{}, {9002}});

    TS_ASSERT(correctSpectraMapping(*workspace, m_log));
    TS_ASSERT(workspace->getSpectrum(0).getDetectorIDs().empty());
    assertDetectorIDs(*workspace, 1, {2});
  }

  void test_spectrum_numbers_and_counts_are_untouched_by_the_correction() {
    auto workspace = createWorkspace(2, {1, 2}, {{9001}, {9002}});
    auto const countsBefore = workspace->y(0).rawData();
    auto const binsBefore = workspace->x(0).rawData();

    TS_ASSERT(correctSpectraMapping(*workspace, m_log));
    TS_ASSERT_EQUALS(1, workspace->getSpectrum(0).getSpectrumNo());
    TS_ASSERT_EQUALS(2, workspace->getSpectrum(1).getSpectrumNo());
    TS_ASSERT_EQUALS(countsBefore, workspace->y(0).rawData());
    TS_ASSERT_EQUALS(binsBefore, workspace->x(0).rawData());
  }

  void test_monitors_are_available_to_map_onto() {
    // Monitors are detectors as far as the mapping is concerned, so a monitor spectrum whose table entry is unusable
    // is corrected like any other.
    auto workspace = createWorkspace(2, {2}, {{9001}, {9002}}, "instrument", {1});

    TS_ASSERT(correctSpectraMapping(*workspace, m_log));
    assertDetectorIDs(*workspace, 0, {1});
    assertDetectorIDs(*workspace, 1, {2});
  }

private:
  /// Builds a workspace whose instrument defines one detector per entry of detectorIDs, and whose spectra hold the
  /// detector IDs a data file's table would have given them. A non-empty mappingSource is set as the instrument's
  /// "spectra-map-source" parameter. IDs listed in monitorIDs become monitors rather than detectors.
  MatrixWorkspace_sptr createWorkspace(const std::size_t histograms, const std::vector<detid_t> &detectorIDs,
                                       const std::vector<std::set<detid_t>> &fileMapping,
                                       const std::string &mappingSource = "instrument",
                                       const std::vector<detid_t> &monitorIDs = {}) {
    auto workspace = WorkspaceCreationHelper::create2DWorkspace(static_cast<int>(histograms), 5);

    auto instrument = std::make_shared<Instrument>("OSIRIS");
    for (auto const detectorID : detectorIDs) {
      auto *detector = new Detector("det" + std::to_string(detectorID), detectorID, nullptr);
      instrument->add(detector);
      instrument->markAsDetector(detector);
    }
    for (auto const monitorID : monitorIDs) {
      auto *monitor = new Detector("mon" + std::to_string(monitorID), monitorID, nullptr);
      instrument->add(monitor);
      instrument->markAsMonitor(monitor);
    }
    workspace->setInstrument(instrument);
    if (!mappingSource.empty())
      workspace->instrumentParameters().addString(instrument.get(), SPECTRA_MAP_SOURCE, mappingSource);

    for (std::size_t i = 0; i < histograms; ++i) {
      workspace->getSpectrum(i).setSpectrumNo(static_cast<specnum_t>(i + 1));
      workspace->getSpectrum(i).setDetectorIDs(fileMapping[i]);
    }
    return workspace;
  }

  void assertDetectorIDs(const MatrixWorkspace &workspace, const std::size_t index, const std::set<detid_t> &expected) {
    TS_ASSERT_EQUALS(expected, workspace.getSpectrum(index).getDetectorIDs());
  }

  Kernel::Logger m_log{"InstrumentSpectraMappingTest"};
};
