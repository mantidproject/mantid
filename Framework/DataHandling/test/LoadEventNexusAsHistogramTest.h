// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Axis.h"
#include "MantidDataHandling/LoadEventNexusAsHistogram.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidFrameworkTestHelpers/FileResource.h"
#include "MantidKernel/Unit.h"

#include <H5Cpp.h>
#include <cxxtest/TestSuite.h>

#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

using Mantid::DataHandling::LoadEventNexusAsHistogram;

class LoadEventNexusAsHistogramTest : public CxxTest::TestSuite {
public:
  static LoadEventNexusAsHistogramTest *createSuite() { return new LoadEventNexusAsHistogramTest(); }
  static void destroySuite(LoadEventNexusAsHistogramTest *suite) { delete suite; }

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
    algorithm.setPropertyValue("TimeBinningStrategy", "Native");
    algorithm.setPropertyValue("AccumulatorType", "UInt32");
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

  void test_parallel_events_double_binning_and_uint64_accumulators_match_expected_counts() {
    FileResource resource("LoadEventNexusAsHistogramFallbacksTest.nxs", false, true);
    createFile(resource.fullPath(), false);

    LoadEventNexusAsHistogram algorithm;
    algorithm.setChild(true);
    algorithm.initialize();
    algorithm.setPropertyValue("Filename", resource.fullPath());
    algorithm.setPropertyValue("OutputWorkspace", "unused");
    algorithm.setProperty("NumberOfSpectra", 2);
    algorithm.setProperty("Params", std::vector<double>{0.0, 16.0, 32.0});
    algorithm.setProperty("ReadSizeFromDisk", 2);
    algorithm.setPropertyValue("ProcessingStrategy", "ParallelEvents");
    algorithm.setPropertyValue("TimeBinningStrategy", "Double");
    algorithm.setPropertyValue("AccumulatorType", "UInt64");
    algorithm.setProperty("LoadInstrument", false);
    algorithm.setProperty("LoadLogs", false);

    TS_ASSERT(algorithm.execute());
    Mantid::DataObjects::Workspace2D_sptr workspace = algorithm.getProperty("OutputWorkspace");
    TS_ASSERT_EQUALS(workspace->y(0)[0], 2.0);
    TS_ASSERT_EQUALS(workspace->y(0)[1], 0.0);
    TS_ASSERT_EQUALS(workspace->y(1)[0], 1.0);
    TS_ASSERT_EQUALS(workspace->y(1)[1], 1.0);
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
    algorithm.setPropertyValue("TimeBinningStrategy", "Native");
    algorithm.setProperty("LoadInstrument", false);
    algorithm.setProperty("LoadLogs", false);

    TS_ASSERT(algorithm.execute());
    Mantid::DataObjects::Workspace2D_sptr workspace = algorithm.getProperty("OutputWorkspace");
    TS_ASSERT_EQUALS(workspace->y(0)[0], 2.0);
    TS_ASSERT_EQUALS(workspace->y(0)[1], 2.0);
  }

  void test_float_event_times_use_double_binning_and_reject_forced_native_binning() {
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

    LoadEventNexusAsHistogram forcedNative;
    forcedNative.setChild(true);
    forcedNative.setRethrows(true);
    forcedNative.initialize();
    forcedNative.setPropertyValue("Filename", resource.fullPath());
    forcedNative.setPropertyValue("OutputWorkspace", "unused");
    forcedNative.setProperty("NumberOfSpectra", 1);
    forcedNative.setProperty("Params", std::vector<double>{0.0, 16.0, 32.0});
    forcedNative.setPropertyValue("TimeBinningStrategy", "Native");
    forcedNative.setProperty("LoadInstrument", false);
    forcedNative.setProperty("LoadLogs", false);
    TS_ASSERT_THROWS(forcedNative.execute(), const std::runtime_error &);
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
