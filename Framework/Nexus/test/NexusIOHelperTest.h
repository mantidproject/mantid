// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/FileFinder.h"
#include "MantidNexus/NexusIOHelper.h"
#include "MantidNexusCpp/NeXusFile.hpp"

#include <cxxtest/TestSuite.h>

namespace Nioh = Mantid::NeXus::NeXusIOHelper;

class NexusIOHelperTest : public CxxTest::TestSuite {

public:
  void test_nexus_io_helper_readNexusVector() {
    const std::string filename = Mantid::API::FileFinder::Instance().getFullPath("V20_ESS_example.nxs");
    ::NeXus::File file(filename);
    file.openGroup("entry", "NXentry");
    file.openGroup("raw_event_data", "NXevent_data");
    auto event_index = Nioh::readNexusVector<uint64_t>(file, "event_index");
    TS_ASSERT_EQUALS(event_index.size(), 1439);
    TS_ASSERT_EQUALS(event_index[100], 100);
    auto event_id = Nioh::readNexusVector<uint64_t>(file, "event_id");
    TS_ASSERT_EQUALS(event_id.size(), 1439);
    TS_ASSERT_EQUALS(event_id[100], 3843);
    auto event_time_offset = Nioh::readNexusVector<float>(file, "event_time_offset");
    TS_ASSERT_EQUALS(event_time_offset.size(), 1439);
    TS_ASSERT_EQUALS(event_time_offset[100], 0.);
    auto event_time_zero = Nioh::readNexusVector<double>(file, "event_time_zero");
    TS_ASSERT_EQUALS(event_time_zero.size(), 1439);
    TS_ASSERT_EQUALS(event_time_zero[100], 1543584891250635008.0);
    file.closeGroup();
    file.closeGroup();
  }

  void test_nexus_io_helper_readNexusVector_out_buffer() {
    const std::string filename = Mantid::API::FileFinder::Instance().getFullPath("V20_ESS_example.nxs");
    ::NeXus::File file(filename);
    file.openGroup("entry", "NXentry");
    file.openGroup("raw_event_data", "NXevent_data");
    file.openData("event_index");
    const auto info = file.getInfo();
    const auto size = std::accumulate(info.dims.begin(), info.dims.end(), int64_t{1}, std::multiplies<>());
    std::vector<uint64_t> event_index(size - 1);
    TS_ASSERT_THROWS_EQUALS(Nioh::readNexusVector(event_index, file, "event_index"), std::runtime_error & e,
                            std::string(e.what()),
                            "The output buffer is too small in NeXusIOHelper::readNexusAnyVector");
    event_index.resize(size);
    Nioh::readNexusVector(event_index, file, "event_index");
    file.closeData();
    TS_ASSERT_EQUALS(event_index[100], 100);
    std::vector<uint64_t> event_id(size);
    Nioh::readNexusVector(event_id, file, "event_id");
    TS_ASSERT_EQUALS(event_id[100], 3843);
    std::vector<float> event_time_offset(size);
    Nioh::readNexusVector(event_time_offset, file, "event_time_offset");
    TS_ASSERT_EQUALS(event_time_offset[100], 0.);
    std::vector<double> event_time_zero(size);
    Nioh::readNexusVector(event_time_zero, file, "event_time_zero");
    TS_ASSERT_EQUALS(event_time_zero[100], 1543584891250635008.0);
    file.closeGroup();
    file.closeGroup();
  }

  void test_nexus_io_helper_readNexusVector_throws_when_narrowing() {
    const std::string filename = Mantid::API::FileFinder::Instance().getFullPath("V20_ESS_example.nxs");
    ::NeXus::File file(filename);
    file.openGroup("entry", "NXentry");
    file.openGroup("raw_event_data", "NXevent_data");
    auto event_index = Nioh::readNexusVector<uint64_t>(file, "event_index");
    TS_ASSERT_EQUALS(event_index.size(), 1439);
    TS_ASSERT_THROWS_EQUALS(auto event_id = Nioh::readNexusVector<uint16_t>(file, "event_id"), std::runtime_error & e,
                            std::string(e.what()), "Narrowing is forbidden in NeXusIOHelper::readNexusAnyVector");
    TS_ASSERT_THROWS_EQUALS(auto event_time_offset = Nioh::readNexusVector<uint16_t>(file, "event_time_offset"),
                            std::runtime_error & e, std::string(e.what()),
                            "Narrowing is forbidden in NeXusIOHelper::readNexusAnyVector");
    TS_ASSERT_THROWS_EQUALS(auto event_time_zero = Nioh::readNexusVector<float>(file, "event_time_zero"),
                            std::runtime_error & e, std::string(e.what()),
                            "Narrowing is forbidden in NeXusIOHelper::readNexusAnyVector");
    file.closeGroup();
    file.closeGroup();
  }

  void test_nexus_io_helper_readNexusVector_allow_narrowing() {
    const std::string filename = Mantid::API::FileFinder::Instance().getFullPath("V20_ESS_example.nxs");
    ::NeXus::File file(filename);
    file.openGroup("entry", "NXentry");
    file.openGroup("raw_event_data", "NXevent_data");
    auto event_index = Nioh::readNexusVector<uint32_t, Nioh::AllowNarrowing>(file, "event_index");
    TS_ASSERT_EQUALS(event_index.size(), 1439);
    TS_ASSERT_EQUALS(event_index[100], 100);
    auto event_id = Nioh::readNexusVector<uint32_t, Nioh::AllowNarrowing>(file, "event_id");
    TS_ASSERT_EQUALS(event_id.size(), 1439);
    TS_ASSERT_EQUALS(event_id[100], 3843);
    auto event_time_offset = Nioh::readNexusVector<uint16_t, Nioh::AllowNarrowing>(file, "event_time_offset");
    TS_ASSERT_EQUALS(event_time_offset.size(), 1439);
    TS_ASSERT_EQUALS(event_time_offset[100], 0.);
    auto event_time_zero = Nioh::readNexusVector<float, Nioh::AllowNarrowing>(file, "event_time_zero");
    TS_ASSERT_EQUALS(event_time_zero.size(), 1439);
    TS_ASSERT_DIFFERS(event_time_zero[100], 1543584891250635008.0);
    file.closeGroup();
    file.closeGroup();
  }

  void test_nexus_io_helper_readNexusVector_v20_ess_integration_2018() {
    const std::string filename =
        Mantid::API::FileFinder::Instance().getFullPath("V20_ESSIntegration_2018-12-13_0942.nxs");
    ::NeXus::File file(filename);
    file.openGroup("entry", "NXentry");
    file.openGroup("event_data", "NXevent_data");
    auto event_index = Nioh::readNexusVector<uint64_t>(file, "event_index");
    TS_ASSERT_EQUALS(event_index.size(), 38258);
    auto event_id = Nioh::readNexusVector<uint64_t>(file, "event_id");
    TS_ASSERT_EQUALS(event_id.size(), 43277);
    auto event_time_offset = Nioh::readNexusVector<float>(file, "event_time_offset");
    TS_ASSERT_EQUALS(event_time_offset.size(), 43277);
    auto event_time_zero = Nioh::readNexusVector<double>(file, "event_time_zero");
    TS_ASSERT_EQUALS(event_time_zero.size(), 38258);
    file.closeGroup();
    file.closeGroup();
  }

  void test_nexus_io_helper_readNexusSlab() {
    const std::string filename = Mantid::API::FileFinder::Instance().getFullPath("V20_ESS_example.nxs");
    ::NeXus::File file(filename);
    file.openGroup("entry", "NXentry");
    file.openGroup("raw_event_data", "NXevent_data");
    auto event_index = Nioh::readNexusSlab<uint64_t>(file, "event_index", {10}, {200});
    TS_ASSERT_EQUALS(event_index.size(), 200);
    auto event_id = Nioh::readNexusSlab<uint64_t>(file, "event_id", {100}, {300});
    TS_ASSERT_EQUALS(event_id.size(), 300);
    auto event_time_offset = Nioh::readNexusSlab<float>(file, "event_time_offset", {1000}, {400});
    TS_ASSERT_EQUALS(event_time_offset.size(), 400);
    auto event_time_zero = Nioh::readNexusSlab<double>(file, "event_time_zero", {111}, {501});
    TS_ASSERT_EQUALS(event_time_zero.size(), 501);
    file.closeGroup();
    file.closeGroup();
  }

  void test_nexus_io_helper_readNexusSlab_out_buffer() {
    const std::string filename = Mantid::API::FileFinder::Instance().getFullPath("V20_ESS_example.nxs");
    ::NeXus::File file(filename);
    file.openGroup("entry", "NXentry");
    file.openGroup("raw_event_data", "NXevent_data");
    std::vector<uint64_t> event_index(200);
    Nioh::readNexusSlab(event_index, file, "event_index", {10}, {200});
    TS_ASSERT_EQUALS(event_index[90], 100);
    std::vector<uint64_t> event_id(300);
    Nioh::readNexusSlab(event_id, file, "event_id", {100}, {300});
    TS_ASSERT_EQUALS(event_id[0], 3843);
    std::vector<float> event_time_offset(400);
    Nioh::readNexusSlab(event_time_offset, file, "event_time_offset", {1000}, {400});
    TS_ASSERT_EQUALS(event_time_offset[200], 0.);
    std::vector<double> event_time_zero(501);
    Nioh::readNexusSlab<double>(event_time_zero, file, "event_time_zero", {111}, {501});
    TS_ASSERT_EQUALS(event_time_zero[100], 1543585007190292224.0);
    file.closeGroup();
    file.closeGroup();
  }

  void test_nexus_io_helper_readNexusSlab_throws_when_Narrowing() {
    const std::string filename = Mantid::API::FileFinder::Instance().getFullPath("V20_ESS_example.nxs");
    ::NeXus::File file(filename);
    file.openGroup("entry", "NXentry");
    file.openGroup("raw_event_data", "NXevent_data");
    auto event_index = Nioh::readNexusSlab<uint64_t>(file, "event_index", {111}, {222});
    TS_ASSERT_EQUALS(event_index.size(), 222);
    TS_ASSERT_THROWS_EQUALS(auto event_id = Nioh::readNexusSlab<uint16_t>(file, "event_id", {222}, {333}),
                            std::runtime_error & e, std::string(e.what()),
                            "Narrowing is forbidden in NeXusIOHelper::readNexusAnySlab");
    TS_ASSERT_THROWS_EQUALS(
        auto event_time_offset = Nioh::readNexusSlab<uint16_t>(file, "event_time_offset", {333}, {444}),
        std::runtime_error & e, std::string(e.what()), "Narrowing is forbidden in NeXusIOHelper::readNexusAnySlab");
    TS_ASSERT_THROWS_EQUALS(auto event_time_zero = Nioh::readNexusSlab<float>(file, "event_time_zero", {444}, {555}),
                            std::runtime_error & e, std::string(e.what()),
                            "Narrowing is forbidden in NeXusIOHelper::readNexusAnySlab");
    file.closeGroup();
    file.closeGroup();
  }

  void test_nexus_io_helper_readNexusSlab_v20_ess_integration_2018() {
    const std::string filename =
        Mantid::API::FileFinder::Instance().getFullPath("V20_ESSIntegration_2018-12-13_0942.nxs");
    ::NeXus::File file(filename);
    file.openGroup("entry", "NXentry");
    file.openGroup("event_data", "NXevent_data");
    auto event_index = Nioh::readNexusSlab<uint64_t>(file, "event_index", {111}, {222});
    TS_ASSERT_EQUALS(event_index.size(), 222);
    auto event_id = Nioh::readNexusSlab<uint64_t>(file, "event_id", {333}, {444});
    TS_ASSERT_EQUALS(event_id.size(), 444);
    auto event_time_offset = Nioh::readNexusSlab<float>(file, "event_time_offset", {555}, {666});
    TS_ASSERT_EQUALS(event_time_offset.size(), 666);
    auto event_time_zero = Nioh::readNexusSlab<double>(file, "event_time_zero", {777}, {888});
    TS_ASSERT_EQUALS(event_time_zero.size(), 888);
    file.closeGroup();
    file.closeGroup();
  }

  void test_nexus_io_helper_readNexusValue() {
    const std::string filename = Mantid::API::FileFinder::Instance().getFullPath("LARMOR00003368.nxs");
    ::NeXus::File file(filename);
    file.openGroup("raw_data_1", "NXentry");
    file.openGroup("monitor_1", "NXmonitor");
    auto monitor_number = Nioh::readNexusValue<int32_t>(file, "monitor_number");
    TS_ASSERT_EQUALS(monitor_number, 1);
    TS_ASSERT_THROWS(Nioh::readNexusValue<int16_t>(file, "monitor_number"),
                     std::runtime_error &);                                          // Narrowing forbidden
    TS_ASSERT_THROWS_NOTHING(Nioh::readNexusValue<int64_t>(file, "monitor_number")); // Larger OK
    file.close();
  }
};
