// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Axis.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataHandling/LoadEventNexusAsHistogram.h"
#include "MantidDataHandling/LoadInstrument.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidFrameworkTestHelpers/FileResource.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidKernel/OptionalBool.h"
#include "MantidKernel/Unit.h"
#include "MantidNexus/NexusFile.h"

#include <H5Cpp.h>
#include <cxxtest/TestSuite.h>

#include <cmath>
#include <cstdint>
#include <limits>
#include <set>
#include <string>
#include <vector>

using Mantid::DataHandling::LoadEventNexusAsHistogram;

class LoadEventNexusAsHistogramTest : public CxxTest::TestSuite {
public:
  static LoadEventNexusAsHistogramTest *createSuite() { return new LoadEventNexusAsHistogramTest(); }
  static void destroySuite(LoadEventNexusAsHistogramTest *suite) { delete suite; }

  void test_default_instrument_loading_maps_physical_ids() {
    FileResource resource("LoadEventNexusAsHistogramDefaultInstrumentMapping.nxs", false, true);
    createInstrumentFile(resource.fullPath(), 20, {20, 10, 11});
    LoadEventNexusAsHistogram algorithm;
    algorithm.setChild(true);
    algorithm.setRethrows(true);
    algorithm.initialize();
    TS_ASSERT_EQUALS(algorithm.getPropertyValue("LoadInstrument"), "1");
    algorithm.setPropertyValue("Filename", resource.fullPath());
    algorithm.setPropertyValue("OutputWorkspace", "unused");
    algorithm.setProperty("Params", std::vector<double>{0.0, 16.0, 32.0});
    algorithm.setProperty("LoadLogs", false);
    TS_ASSERT(algorithm.execute());
    Mantid::DataObjects::Workspace2D_sptr workspace = algorithm.getProperty("OutputWorkspace");
    TS_ASSERT_EQUALS(workspace->getNumberHistograms(), 2);
    TS_ASSERT_EQUALS(workspace->getSpectrum(0).getDetectorIDs(), std::set<Mantid::detid_t>{10});
    TS_ASSERT_EQUALS(workspace->getSpectrum(1).getDetectorIDs(), std::set<Mantid::detid_t>{20});
    TS_ASSERT_EQUALS(workspace->y(0)[0], 1.0);
    TS_ASSERT_EQUALS(workspace->y(1)[0], 1.0);
  }

  void test_pulse_height_filter_can_be_disabled_for_both_processing_paths_and_binning_modes() {
    FileResource resource("LoadEventNexusAsHistogramPulseHeightSwitch.nxs", false, true);
    {
      H5::H5File file(resource.fullPath(), H5F_ACC_TRUNC);
      auto entry = file.createGroup("entry");
      writeStringAttribute(entry, "NX_class", "NXentry");
      addEventGroup(
          entry, "events", {0, 0, 0, 0, 1, 0}, {1000, 2000, 3000, 4000, 1000, 40000},
          {0.0, -1.0, std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::infinity(), 1.0, 1.0},
          {0});
    }
    for (const auto &strategy : {"Pipeline", "ParallelEvents"}) {
      for (const double binWidth : {16.0, 15.9995}) {
        for (const bool filter : {true, false}) {
          LoadEventNexusAsHistogram algorithm;
          configure(algorithm, resource.fullPath());
          TS_ASSERT_EQUALS(algorithm.getPropertyValue("FilterByPulseHeight"), "1");
          algorithm.setProperty("FilterByPulseHeight", filter);
          algorithm.setPropertyValue("ProcessingStrategy", strategy);
          algorithm.setProperty("Params", std::vector<double>{0.0, binWidth, 2 * binWidth});
          TS_ASSERT(algorithm.execute());
          Mantid::DataObjects::Workspace2D_sptr workspace = algorithm.getProperty("OutputWorkspace");
          TS_ASSERT_EQUALS(workspace->y(0)[0], filter ? 1.0 : 4.0);
          TS_ASSERT_EQUALS(workspace->y(0)[1], 0.0);
          TS_ASSERT_EQUALS(workspace->e(0)[0], filter ? 1.0 : 2.0);
        }
      }
    }
  }

  void test_disabled_pulse_height_filter_does_not_read_or_validate_unused_dataset() {
    FileResource resource("LoadEventNexusAsHistogramIgnoredPulseHeight.nxs", false, true);
    {
      H5::H5File file(resource.fullPath(), H5F_ACC_TRUNC);
      auto entry = file.createGroup("entry");
      writeStringAttribute(entry, "NX_class", "NXentry");
      addEventGroup(entry, "events", {0, 0}, {1000, 17000}, {1.0}, {0});
    }
    LoadEventNexusAsHistogram algorithm;
    configure(algorithm, resource.fullPath());
    algorithm.setProperty("FilterByPulseHeight", false);
    TS_ASSERT(algorithm.execute());
    Mantid::DataObjects::Workspace2D_sptr workspace = algorithm.getProperty("OutputWorkspace");
    TS_ASSERT_EQUALS(workspace->y(0)[0], 1.0);
    TS_ASSERT_EQUALS(workspace->y(0)[1], 1.0);
  }

  void test_instrument_mapping_supports_dense_gaps_sparse_ids_and_excludes_monitors() {
    for (const uint32_t secondID : {20U, 1000000000U}) {
      FileResource resource("LoadEventNexusAsHistogramInstrumentIDs.nxs", false, true);
      createInstrumentFile(resource.fullPath(), secondID, {secondID, 10, 11});
      for (const auto &strategy : {"Pipeline", "ParallelEvents"}) {
        for (const double binWidth : {16.0, 15.9995}) {
          LoadEventNexusAsHistogram algorithm;
          configure(algorithm, resource.fullPath());
          algorithm.setProperty("LoadInstrument", true);
          algorithm.setPropertyValue("ProcessingStrategy", strategy);
          algorithm.setProperty("Params", std::vector<double>{0.0, binWidth, 2 * binWidth});
          // The default of 64 is not used; derive two non-monitor spectra.
          algorithm.setProperty("NumberOfSpectra", 64);
          TS_ASSERT(algorithm.execute());
          Mantid::DataObjects::Workspace2D_sptr workspace = algorithm.getProperty("OutputWorkspace");
          TS_ASSERT_EQUALS(workspace->getNumberHistograms(), 2);
          TS_ASSERT_EQUALS(workspace->getSpectrum(0).getDetectorIDs(), std::set<Mantid::detid_t>{10});
          TS_ASSERT_EQUALS(workspace->getSpectrum(1).getDetectorIDs(),
                           std::set<Mantid::detid_t>{static_cast<int>(secondID)});
          TS_ASSERT_EQUALS(workspace->getSpectrum(0).getSpectrumNo(), 1);
          TS_ASSERT_EQUALS(workspace->getSpectrum(1).getSpectrumNo(), 2);
          TS_ASSERT_EQUALS(workspace->y(0)[0], 1.0);
          TS_ASSERT_EQUALS(workspace->y(1)[0], 1.0);
          TS_ASSERT_EQUALS(workspace->detectorInfo().isMonitor(workspace->detectorInfo().indexOf(11)), true);
        }
      }
    }
  }

  void test_instrument_mapping_rejects_legacy_data_before_loading_metadata() {
    for (const std::string layout : {"empty", "SPEC", "UDET", "complete"}) {
      FileResource resource("LoadEventNexusAsHistogramLegacyMapping.nxs", false, true);
      createInstrumentFile(resource.fullPath(), 20, {10});
      {
        H5::H5File file(resource.fullPath(), H5F_ACC_RDWR);
        auto vms = file.openGroup("entry").createGroup("isis_vms_compat");
        if (layout == "SPEC" || layout == "complete")
          writeUIntDataset(vms, "SPEC", {1});
        if (layout == "UDET" || layout == "complete")
          writeUIntDataset(vms, "UDET", {10});
        // No instrument is available: reject the file before attempting to load one.
        file.openGroup("entry").unlink("instrument");
      }
      LoadEventNexusAsHistogram algorithm;
      configure(algorithm, resource.fullPath());
      algorithm.setProperty("LoadInstrument", true);
      TS_ASSERT_THROWS_EQUALS(algorithm.execute(), const std::runtime_error &error, std::string(error.what()),
                              "LoadInstrument=True does not support legacy isis_vms_compat mapping.");
    }
  }

  void test_disabled_instrument_loading_uses_arithmetic_mapping_and_ignores_legacy_data() {
    for (const std::string layout : {"empty", "SPEC", "UDET", "complete"}) {
      FileResource resource("LoadEventNexusAsHistogramIgnoredLegacyMapping.nxs", false, true);
      createInstrumentFile(resource.fullPath(), 20, {10, 11});
      {
        H5::H5File file(resource.fullPath(), H5F_ACC_RDWR);
        auto vms = file.openGroup("entry").createGroup("isis_vms_compat");
        // Deliberately unrelated to the instrument and event IDs: these must not be interpreted.
        if (layout == "SPEC" || layout == "complete")
          writeUIntDataset(vms, "SPEC", {100});
        if (layout == "UDET" || layout == "complete")
          writeUIntDataset(vms, "UDET", {9999});
      }
      LoadEventNexusAsHistogram algorithm;
      configure(algorithm, resource.fullPath());
      algorithm.setProperty("NumberOfSpectra", 2);
      algorithm.setProperty("EventIDOffset", 10);
      TS_ASSERT(algorithm.execute());
      Mantid::DataObjects::Workspace2D_sptr workspace = algorithm.getProperty("OutputWorkspace");
      TS_ASSERT_EQUALS(workspace->y(0)[0], 1.0);
      TS_ASSERT_EQUALS(workspace->y(1)[0], 1.0);
      TS_ASSERT_EQUALS(workspace->e(0)[0], 1.0);
      TS_ASSERT_EQUALS(workspace->e(1)[0], 1.0);
      TS_ASSERT_EQUALS(workspace->getSpectrum(0).getDetectorIDs(), std::set<Mantid::detid_t>{1});
      TS_ASSERT_EQUALS(workspace->getSpectrum(1).getDetectorIDs(), std::set<Mantid::detid_t>{2});
      TS_ASSERT(workspace->getInstrument()->getDetectorIDs().empty());
    }
  }

  void test_legacy_mapping_in_an_unselected_entry_does_not_prevent_loading() {
    FileResource resource("LoadEventNexusAsHistogramUnselectedLegacyEntry.nxs", false, true);
    createInstrumentFile(resource.fullPath(), 20, {10});
    {
      H5::H5File file(resource.fullPath(), H5F_ACC_RDWR);
      auto other = file.createGroup("other");
      writeStringAttribute(other, "NX_class", "NXentry");
      other.createGroup("isis_vms_compat");
      addEventGroup(other, "events", {1}, {1000}, {1.0}, {0});
    }
    LoadEventNexusAsHistogram algorithm;
    configure(algorithm, resource.fullPath());
    algorithm.setPropertyValue("NXentryName", "entry");
    algorithm.setProperty("LoadInstrument", true);
    algorithm.setProperty("NumberOfSpectra", 2);
    TS_ASSERT(algorithm.execute());
    Mantid::DataObjects::Workspace2D_sptr workspace = algorithm.getProperty("OutputWorkspace");
    TS_ASSERT_EQUALS(workspace->y(0)[0], 1.0);
    TS_ASSERT_EQUALS(workspace->y(1)[0], 0.0);
  }

  void test_instrument_mapping_rejects_unknown_event_ids_instead_of_guessing_an_offset() {
    FileResource resource("LoadEventNexusAsHistogramUnknownID.nxs", false, true);
    createInstrumentFile(resource.fullPath(), 20, {0});
    LoadEventNexusAsHistogram algorithm;
    configure(algorithm, resource.fullPath());
    algorithm.setProperty("LoadInstrument", true);
    algorithm.setProperty("NumberOfSpectra", 64);
    TS_ASSERT_THROWS_EQUALS(algorithm.execute(), const std::runtime_error &error, std::string(error.what()),
                            "Event ID 0 is not present in the instrument mapping.");
  }

  void test_event_id_offset_requires_instrument_loading_to_be_disabled() {
    LoadEventNexusAsHistogram algorithm;
    algorithm.initialize();
    algorithm.setProperty("EventIDOffset", 1);
    const auto errors = static_cast<Mantid::API::Algorithm &>(algorithm).validateInputs();
    TS_ASSERT(errors.contains("EventIDOffset"));
    algorithm.setProperty("LoadInstrument", false);
    TS_ASSERT(!static_cast<Mantid::API::Algorithm &>(algorithm).validateInputs().contains("EventIDOffset"));
  }

  void test_instrument_mapping_rejects_conflicting_spectrum_count() {
    FileResource resource("LoadEventNexusAsHistogramWrongSpectrumCount.nxs", false, true);
    createInstrumentFile(resource.fullPath(), 20, {10});
    LoadEventNexusAsHistogram algorithm;
    configure(algorithm, resource.fullPath());
    algorithm.setProperty("LoadInstrument", true);
    algorithm.setProperty("NumberOfSpectra", 5);
    TS_ASSERT_THROWS_EQUALS(algorithm.execute(), const std::runtime_error &error, std::string(error.what()),
                            "NumberOfSpectra does not match the instrument mapping.");
  }

  void test_loads_multiple_event_groups_directly_into_histograms() {
    FileResource resource("LoadEventNexusAsHistogramTest.nxs", false, true);
    createFile(resource.fullPath(), false);

    LoadEventNexusAsHistogram algorithm;
    algorithm.setChild(true);
    algorithm.initialize();
    algorithm.setPropertyValue("Filename", resource.fullPath());
    algorithm.setPropertyValue("OutputWorkspace", "unused");
    algorithm.setProperty("NumberOfSpectra", 2);
    algorithm.setProperty("Params", std::vector<double>{0.0, 16.0, 32.0});
    algorithm.setProperty("ReadSizeFromDisk", 2);
    algorithm.setPropertyValue("ProcessingStrategy", "Pipeline");
    algorithm.setProperty("MaxInFlightChunks", 2);
    algorithm.setProperty("LoadInstrument", false);
    algorithm.setProperty("LoadLogs", false);

    TS_ASSERT(algorithm.execute());
    Mantid::DataObjects::Workspace2D_sptr workspace = algorithm.getProperty("OutputWorkspace");
    TS_ASSERT(workspace);
    TS_ASSERT_EQUALS(workspace->getNumberHistograms(), 2);
    TS_ASSERT_EQUALS(workspace->blocksize(), 2);
    TS_ASSERT_EQUALS(workspace->getAxis(0)->unit()->unitID(), "TOF");
    TS_ASSERT_EQUALS(workspace->y(0)[0], 2.0);
    TS_ASSERT_EQUALS(workspace->y(0)[1], 0.0);
    TS_ASSERT_EQUALS(workspace->y(1)[0], 1.0);
    TS_ASSERT_EQUALS(workspace->y(1)[1], 1.0);
    TS_ASSERT_EQUALS(workspace->e(0)[0], std::sqrt(2.0));
    TS_ASSERT_EQUALS(workspace->e(1)[0], 1.0);
    TS_ASSERT_EQUALS(workspace->x(0)[0], 0.0);
    TS_ASSERT_EQUALS(workspace->x(0)[1], 16.0);
    TS_ASSERT_EQUALS(workspace->x(0)[2], 32.0);
    TS_ASSERT_EQUALS(workspace->getSpectrum(0).getSpectrumNo(), 1);
    TS_ASSERT_EQUALS(*workspace->getSpectrum(0).getDetectorIDs().cbegin(), 1);
  }

  void test_event_id_offset_is_applied_before_workspace_index_mapping() {
    FileResource resource("LoadEventNexusAsHistogramOffsetTest.nxs", false, true);
    createOffsetFile(resource.fullPath());

    LoadEventNexusAsHistogram algorithm;
    algorithm.setChild(true);
    algorithm.initialize();
    algorithm.setPropertyValue("Filename", resource.fullPath());
    algorithm.setPropertyValue("OutputWorkspace", "unused");
    algorithm.setProperty("NumberOfSpectra", 2);
    algorithm.setProperty("Params", std::vector<double>{0.0, 16.0, 32.0});
    algorithm.setProperty("EventIDOffset", 1);
    algorithm.setProperty("LoadInstrument", false);
    algorithm.setProperty("LoadLogs", false);

    TS_ASSERT(algorithm.execute());
    Mantid::DataObjects::Workspace2D_sptr workspace = algorithm.getProperty("OutputWorkspace");
    TS_ASSERT_EQUALS(workspace->y(0)[0], 1.0);
    TS_ASSERT_EQUALS(workspace->y(1)[1], 1.0);
  }

  void test_processing_strategies_produce_expected_histograms_with_automatic_binning() {
    FileResource resource("LoadEventNexusAsHistogramStrategiesTest.nxs", false, true);
    createFile(resource.fullPath(), false);
    {
      H5::H5File file(resource.fullPath(), H5F_ACC_RDWR);
      auto entry = file.openGroup("raw_data_1");
      addEventGroup(entry, "empty_events", {}, {}, {}, {});
    }

    for (const auto &strategy : {"Auto", "Pipeline", "ParallelEvents"}) {
      for (const double binWidth : {16.0, 15.9995}) {
        LoadEventNexusAsHistogram algorithm;
        algorithm.setChild(true);
        algorithm.initialize();
        algorithm.setPropertyValue("Filename", resource.fullPath());
        algorithm.setPropertyValue("OutputWorkspace", "unused");
        algorithm.setProperty("NumberOfSpectra", 2);
        algorithm.setProperty("Params", std::vector<double>{0.0, binWidth, 2 * binWidth});
        algorithm.setProperty("ReadSizeFromDisk", 2);
        algorithm.setProperty("MaxInFlightChunks", 2);
        algorithm.setPropertyValue("ProcessingStrategy", strategy);
        algorithm.setProperty("LoadInstrument", false);
        algorithm.setProperty("LoadLogs", false);

        TS_ASSERT(algorithm.execute());
        Mantid::DataObjects::Workspace2D_sptr workspace = algorithm.getProperty("OutputWorkspace");
        TS_ASSERT_EQUALS(workspace->y(0)[0], 2.0);
        TS_ASSERT_EQUALS(workspace->y(0)[1], 0.0);
        TS_ASSERT_EQUALS(workspace->y(1)[0], 1.0);
        TS_ASSERT_EQUALS(workspace->y(1)[1], 1.0);
        TS_ASSERT_EQUALS(workspace->e(0)[0], std::sqrt(2.0));
        TS_ASSERT_EQUALS(workspace->e(0)[1], 0.0);
        TS_ASSERT_EQUALS(workspace->e(1)[0], 1.0);
        TS_ASSERT_EQUALS(workspace->e(1)[1], 1.0);
      }
    }
  }

  void test_rejects_mismatched_event_dataset_lengths() {
    for (const std::string dataset : {"event_time_offset", "pulse_height"}) {
      FileResource resource("LoadEventNexusAsHistogramDatasetLengthsTest.nxs", false, true);
      {
        H5::H5File file(resource.fullPath(), H5F_ACC_TRUNC);
        auto entry = file.createGroup("entry");
        writeStringAttribute(entry, "NX_class", "NXentry");
        const auto times =
            dataset == "event_time_offset" ? std::vector<uint32_t>{1000} : std::vector<uint32_t>{1000, 17000};
        const auto heights = dataset == "pulse_height" ? std::vector<double>{1.0} : std::vector<double>{1.0, 1.0};
        addEventGroup(entry, "events", {0, 1}, times, heights, {0});
      }

      LoadEventNexusAsHistogram algorithm;
      algorithm.setChild(true);
      algorithm.setRethrows(true);
      algorithm.initialize();
      algorithm.setPropertyValue("Filename", resource.fullPath());
      algorithm.setPropertyValue("OutputWorkspace", "unused");
      algorithm.setProperty("LoadInstrument", false);
      algorithm.setProperty("LoadLogs", false);

      const std::string expected = dataset == "event_time_offset"
                                       ? "event_id and event_time_offset have different lengths in '/entry/events'."
                                       : "pulse_height has a different length from event_id in '/entry/events'.";
      TS_ASSERT_THROWS_EQUALS(algorithm.execute(), const std::runtime_error &error, std::string(error.what()),
                              expected);
    }
  }

  void test_prefers_raw_data_entry_and_excludes_error_and_unmapped_events() {
    FileResource resource("LoadEventNexusAsHistogramEntrySelectionTest.nxs", false, true);
    createFile(resource.fullPath(), false);
    {
      H5::H5File file(resource.fullPath(), H5F_ACC_RDWR);
      auto preferred = file.openGroup("raw_data_1");
      addEventGroup(preferred, "bank_error_events", {0}, {1000}, {1.0}, {0});
      addEventGroup(preferred, "bank_unmapped_events", {0}, {1000}, {1.0}, {0});
      auto other = file.createGroup("aaa");
      writeStringAttribute(other, "NX_class", "NXentry");
      addEventGroup(other, "events", {0}, {17000}, {1.0}, {0});
    }

    for (const auto &entry : {std::string{}, std::string{"/aaa"}}) {
      LoadEventNexusAsHistogram algorithm;
      algorithm.setChild(true);
      algorithm.initialize();
      algorithm.setPropertyValue("Filename", resource.fullPath());
      algorithm.setPropertyValue("OutputWorkspace", "unused");
      algorithm.setPropertyValue("NXentryName", entry);
      algorithm.setProperty("NumberOfSpectra", 2);
      algorithm.setProperty("Params", std::vector<double>{0.0, 16.0, 32.0});
      algorithm.setProperty("LoadInstrument", false);
      algorithm.setProperty("LoadLogs", false);

      TS_ASSERT(algorithm.execute());
      Mantid::DataObjects::Workspace2D_sptr workspace = algorithm.getProperty("OutputWorkspace");
      TS_ASSERT_EQUALS(workspace->y(0)[0], entry.empty() ? 2.0 : 0.0);
      TS_ASSERT_EQUALS(workspace->y(0)[1], entry.empty() ? 0.0 : 1.0);
      TS_ASSERT_EQUALS(workspace->y(1)[0], entry.empty() ? 1.0 : 0.0);
      TS_ASSERT_EQUALS(workspace->y(1)[1], entry.empty() ? 1.0 : 0.0);
    }
  }

  void test_missing_pulse_height_does_not_apply_amplitude_filter() {
    FileResource resource("LoadEventNexusAsHistogramNoPulseHeightTest.nxs", false, true);
    createFileWithoutPulseHeight(resource.fullPath());

    LoadEventNexusAsHistogram algorithm;
    algorithm.setChild(true);
    algorithm.initialize();
    algorithm.setPropertyValue("Filename", resource.fullPath());
    algorithm.setPropertyValue("OutputWorkspace", "unused");
    algorithm.setProperty("NumberOfSpectra", 1);
    algorithm.setProperty("Params", std::vector<double>{0.0, 16.0, 32.0});
    algorithm.setProperty("MinimumPulseHeight", 100.0);
    algorithm.setProperty("LoadInstrument", false);
    algorithm.setProperty("LoadLogs", false);

    TS_ASSERT(algorithm.execute());
    Mantid::DataObjects::Workspace2D_sptr workspace = algorithm.getProperty("OutputWorkspace");
    TS_ASSERT_EQUALS(workspace->y(0)[0], 1.0);
  }

  void test_integer_event_times_with_fractional_native_bin_width_use_double_binning() {
    FileResource resource("LoadEventNexusAsHistogramFallbacksTest.nxs", false, true);
    createFile(resource.fullPath(), false);

    LoadEventNexusAsHistogram algorithm;
    algorithm.setChild(true);
    algorithm.initialize();
    algorithm.setPropertyValue("Filename", resource.fullPath());
    algorithm.setPropertyValue("OutputWorkspace", "unused");
    algorithm.setProperty("NumberOfSpectra", 2);
    algorithm.setProperty("Params", std::vector<double>{0.0, 15.9995, 31.999});
    algorithm.setProperty("ReadSizeFromDisk", 2);
    algorithm.setPropertyValue("ProcessingStrategy", "ParallelEvents");
    algorithm.setProperty("LoadInstrument", false);
    algorithm.setProperty("LoadLogs", false);

    TS_ASSERT(algorithm.execute());
    Mantid::DataObjects::Workspace2D_sptr workspace = algorithm.getProperty("OutputWorkspace");
    TS_ASSERT_EQUALS(workspace->y(0)[0], 2.0);
    TS_ASSERT_EQUALS(workspace->y(0)[1], 0.0);
    TS_ASSERT_EQUALS(workspace->y(1)[0], 1.0);
    TS_ASSERT_EQUALS(workspace->y(1)[1], 1.0);
    TS_ASSERT_EQUALS(workspace->x(0)[1], 15.9995);
  }

  void test_native_binning_reads_uint64_event_times_without_converting_to_double() {
    FileResource resource("LoadEventNexusAsHistogramUInt64TimesTest.nxs", false, true);
    createUInt64TimeFile(resource.fullPath());

    LoadEventNexusAsHistogram algorithm;
    algorithm.setChild(true);
    algorithm.initialize();
    algorithm.setPropertyValue("Filename", resource.fullPath());
    algorithm.setPropertyValue("OutputWorkspace", "unused");
    algorithm.setProperty("NumberOfSpectra", 1);
    algorithm.setProperty("Params", std::vector<double>{0.0, 16.0, 32.0});
    algorithm.setProperty("LoadInstrument", false);
    algorithm.setProperty("LoadLogs", false);

    TS_ASSERT(algorithm.execute());
    Mantid::DataObjects::Workspace2D_sptr workspace = algorithm.getProperty("OutputWorkspace");
    TS_ASSERT_EQUALS(workspace->y(0)[0], 2.0);
    TS_ASSERT_EQUALS(workspace->y(0)[1], 2.0);
  }

  void test_float_event_times_automatically_use_double_binning() {
    FileResource resource("LoadEventNexusAsHistogramFloatTimesTest.nxs", false, true);
    createFloatTimeFile(resource.fullPath());

    LoadEventNexusAsHistogram algorithm;
    algorithm.setChild(true);
    algorithm.initialize();
    algorithm.setPropertyValue("Filename", resource.fullPath());
    algorithm.setPropertyValue("OutputWorkspace", "unused");
    algorithm.setProperty("NumberOfSpectra", 1);
    algorithm.setProperty("Params", std::vector<double>{0.0, 16.0, 32.0});
    algorithm.setProperty("LoadInstrument", false);
    algorithm.setProperty("LoadLogs", false);

    TS_ASSERT(algorithm.execute());
    Mantid::DataObjects::Workspace2D_sptr workspace = algorithm.getProperty("OutputWorkspace");
    TS_ASSERT_EQUALS(workspace->y(0)[0], 2.0);
    TS_ASSERT_EQUALS(workspace->y(0)[1], 2.0);
  }

  void test_rejects_multiple_periods() {
    FileResource resource("LoadEventNexusAsHistogramPeriodTest.nxs", false, true);
    createFile(resource.fullPath(), true);

    LoadEventNexusAsHistogram algorithm;
    algorithm.setChild(true);
    algorithm.setRethrows(true);
    algorithm.initialize();
    algorithm.setPropertyValue("Filename", resource.fullPath());
    algorithm.setPropertyValue("OutputWorkspace", "unused");
    algorithm.setProperty("NumberOfSpectra", 2);
    algorithm.setProperty("LoadInstrument", false);
    algorithm.setProperty("LoadLogs", false);

    TS_ASSERT_THROWS(algorithm.execute(), const std::runtime_error &);
  }

  void test_rejects_non_linear_binning() {
    LoadEventNexusAsHistogram algorithm;
    algorithm.initialize();
    TS_ASSERT_THROWS(algorithm.setProperty("Params", std::vector<double>{0.0, -0.1, 10.0}),
                     const std::invalid_argument &);
  }

private:
  static void configure(LoadEventNexusAsHistogram &algorithm, const std::string &filename) {
    algorithm.setChild(true);
    algorithm.setRethrows(true);
    algorithm.initialize();
    algorithm.setPropertyValue("Filename", filename);
    algorithm.setPropertyValue("OutputWorkspace", "unused");
    algorithm.setProperty("NumberOfSpectra", 1);
    algorithm.setProperty("Params", std::vector<double>{0.0, 16.0, 32.0});
    algorithm.setProperty("ReadSizeFromDisk", 2);
    algorithm.setProperty("LoadInstrument", false);
    algorithm.setProperty("LoadLogs", false);
  }

  static void createInstrumentFile(const std::string &filename, const uint32_t secondID,
                                   const std::vector<uint32_t> &events) {
    {
      H5::H5File file(filename, H5F_ACC_TRUNC);
      auto entry = file.createGroup("entry");
      writeStringAttribute(entry, "NX_class", "NXentry");
      addEventGroup(entry, "events", events, std::vector<uint32_t>(events.size(), 1000),
                    std::vector<double>(events.size(), 1.0), {0});
    }
    const std::string xml =
        std::string{
            R"(<instrument name="EventMappingTest" valid-from="2000-01-01 00:00:00" valid-to="2100-01-01 00:00:00">
          <defaults><length unit="metre"/><angle unit="degree"/></defaults>
          <component type="source"><location z="-10"/></component><type name="source" is="Source"/>
          <component type="sample"><location/></component><type name="sample" is="SamplePos"/>
          <component type="pixel" idlist="detectors"><location x="0" z="2"/><location x="1" z="2"/>)"} +
        R"(</component><type name="pixel" is="detector"/>
          <idlist idname="detectors"><id val="10"/><id val=")" +
        std::to_string(secondID) + R"("/>)" +
        R"(</idlist><component type="monitor" idlist="monitors"><location z="-1"/></component>
          <type name="monitor" is="monitor"/><idlist idname="monitors"><id val="11"/></idlist>
          </instrument>)";
    auto workspace = Mantid::API::WorkspaceFactory::Instance().create("Workspace2D", 1, 2, 1);
    Mantid::DataHandling::LoadInstrument load;
    load.initialize();
    load.setChild(true);
    load.setRethrows(true);
    load.setProperty("Workspace", workspace);
    load.setPropertyValue("InstrumentName", "EventMappingTest");
    load.setPropertyValue("InstrumentXML", xml);
    load.setProperty("RewriteSpectraMap", Mantid::Kernel::OptionalBool(false));
    load.execute();
    Mantid::Nexus::File file(filename, NXaccess::RDWR);
    file.openGroup("entry", "NXentry");
    workspace->saveExperimentInfoNexus(&file, true, false, false);
  }

  static void writeStringAttribute(H5::H5Object &object, const std::string &name, const std::string &value) {
    H5::StrType stringType(H5::PredType::C_S1, 32);
    H5::DataSpace scalar(H5S_SCALAR);
    auto attribute = object.createAttribute(name, stringType, scalar);
    attribute.write(stringType, value);
  }

  static void writeUIntDataset(H5::Group &group, const std::string &name, const std::vector<uint32_t> &values) {
    const hsize_t dimensions[1]{values.size()};
    H5::DataSpace dataSpace(1, dimensions);
    auto dataset = group.createDataSet(name, H5::PredType::NATIVE_UINT, dataSpace);
    dataset.write(values.data(), H5::PredType::NATIVE_UINT);
  }

  static void writeDoubleDataset(H5::Group &group, const std::string &name, const std::vector<double> &values) {
    const hsize_t dimensions[1]{values.size()};
    H5::DataSpace dataSpace(1, dimensions);
    auto dataset = group.createDataSet(name, H5::PredType::NATIVE_DOUBLE, dataSpace);
    dataset.write(values.data(), H5::PredType::NATIVE_DOUBLE);
  }

  static void writeUInt64Dataset(H5::Group &group, const std::string &name, const std::vector<uint64_t> &values) {
    const hsize_t dimensions[1]{values.size()};
    H5::DataSpace dataSpace(1, dimensions);
    auto dataset = group.createDataSet(name, H5::PredType::NATIVE_UINT64, dataSpace);
    dataset.write(values.data(), H5::PredType::NATIVE_UINT64);
  }

  static void writeFloatDataset(H5::Group &group, const std::string &name, const std::vector<float> &values) {
    const hsize_t dimensions[1]{values.size()};
    H5::DataSpace dataSpace(1, dimensions);
    auto dataset = group.createDataSet(name, H5::PredType::NATIVE_FLOAT, dataSpace);
    dataset.write(values.data(), H5::PredType::NATIVE_FLOAT);
  }

  static void addEventGroup(H5::Group &entry, const std::string &name, const std::vector<uint32_t> &ids,
                            const std::vector<uint32_t> &times, const std::vector<double> &pulseHeights,
                            const std::vector<uint32_t> &periods) {
    auto group = entry.createGroup(name);
    writeStringAttribute(group, "NX_class", "NXevent_data");
    writeUIntDataset(group, "event_id", ids);
    writeUIntDataset(group, "event_time_offset", times);
    auto timeDataset = group.openDataSet("event_time_offset");
    writeStringAttribute(timeDataset, "units", "ns");
    writeDoubleDataset(group, "pulse_height", pulseHeights);
    writeUIntDataset(group, "period_number", periods);
  }

  static void createFile(const std::string &filename, const bool multiplePeriods) {
    H5::H5File file(filename, H5F_ACC_TRUNC);
    auto entry = file.createGroup("raw_data_1");
    writeStringAttribute(entry, "NX_class", "NXentry");
    addEventGroup(entry, "detector_1_events", {0, 0, 1, 1, 3, 0}, {0, 15999, 16000, 31999, 1000, 32000},
                  {1.0, 1.0, 1.0, 0.0, 1.0, 1.0},
                  multiplePeriods ? std::vector<uint32_t>{0, 1} : std::vector<uint32_t>{0});
    addEventGroup(entry, "detector_2_events", {1}, {500}, {2.0}, {0});
  }

  static void createOffsetFile(const std::string &filename) {
    H5::H5File file(filename, H5F_ACC_TRUNC);
    auto entry = file.createGroup("entry");
    writeStringAttribute(entry, "NX_class", "NXentry");
    addEventGroup(entry, "events", {1, 2}, {1000, 17000}, {1.0, 1.0}, {0});
  }

  static void createFileWithoutPulseHeight(const std::string &filename) {
    H5::H5File file(filename, H5F_ACC_TRUNC);
    auto entry = file.createGroup("entry");
    writeStringAttribute(entry, "NX_class", "NXentry");
    auto group = entry.createGroup("events");
    writeStringAttribute(group, "NX_class", "NXevent_data");
    writeUIntDataset(group, "event_id", {0});
    writeUIntDataset(group, "event_time_offset", {1000});
    auto timeDataset = group.openDataSet("event_time_offset");
    writeStringAttribute(timeDataset, "units", "ns");
    writeUIntDataset(group, "period_number", {0});
  }

  static void createUInt64TimeFile(const std::string &filename) {
    H5::H5File file(filename, H5F_ACC_TRUNC);
    auto entry = file.createGroup("entry");
    writeStringAttribute(entry, "NX_class", "NXentry");
    auto group = entry.createGroup("events");
    writeStringAttribute(group, "NX_class", "NXevent_data");
    writeUIntDataset(group, "event_id", {0, 0, 0, 0});
    writeUInt64Dataset(group, "event_time_offset", {0, 15999, 16000, 31999});
    auto timeDataset = group.openDataSet("event_time_offset");
    writeStringAttribute(timeDataset, "units", "ns");
    writeUIntDataset(group, "period_number", {0});
  }

  static void createFloatTimeFile(const std::string &filename) {
    H5::H5File file(filename, H5F_ACC_TRUNC);
    auto entry = file.createGroup("entry");
    writeStringAttribute(entry, "NX_class", "NXentry");
    auto group = entry.createGroup("events");
    writeStringAttribute(group, "NX_class", "NXevent_data");
    writeUIntDataset(group, "event_id", {0, 0, 0, 0});
    writeFloatDataset(group, "event_time_offset", {0.F, 15.999F, 16.F, 31.999F});
    auto timeDataset = group.openDataSet("event_time_offset");
    writeStringAttribute(timeDataset, "units", "us");
    writeUIntDataset(group, "period_number", {0});
  }
};
