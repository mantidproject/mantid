// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef NEXUSIOHELPERTEST_H_
#define NEXUSIOHELPERTEST_H_

#include "MantidAPI/FileFinder.h"
#include "MantidNexus/NexusIOHelper.h"
#include <nexus/NeXusFile.hpp>

#include <cxxtest/TestSuite.h>

namespace Nioh = Mantid::NeXus::NeXusIOHelper;

class NexusIOHelperTest : public CxxTest::TestSuite {

public:
  void test_nexus_io_helper_readNexusVector() {
    const std::string filename =
        Mantid::API::FileFinder::Instance().getFullPath("V20_ESS_example.nxs");
    ::NeXus::File file(filename);
    file.openGroup("entry", "NXentry");
    file.openGroup("raw_event_data", "NXevent_data");
    auto event_index = Nioh::readNexusVector<uint64_t>(file, "event_index");
    TS_ASSERT_EQUALS(event_index.size(), 1439);
    auto event_id = Nioh::readNexusVector<uint64_t>(file, "event_id");
    TS_ASSERT_EQUALS(event_id.size(), 1439);
    auto event_time_offset =
        Nioh::readNexusVector<float>(file, "event_time_offset");
    TS_ASSERT_EQUALS(event_time_offset.size(), 1439);
    auto event_time_zero =
        Nioh::readNexusVector<double>(file, "event_time_zero");
    TS_ASSERT_EQUALS(event_time_zero.size(), 1439);
    file.closeGroup();
    file.closeGroup();
  }

  void test_nexus_io_helper_readNexusVector_throws_when_downcasting() {
    const std::string filename =
        Mantid::API::FileFinder::Instance().getFullPath("V20_ESS_example.nxs");
    ::NeXus::File file(filename);
    file.openGroup("entry", "NXentry");
    file.openGroup("raw_event_data", "NXevent_data");
    auto event_index = Nioh::readNexusVector<uint64_t>(file, "event_index");
    TS_ASSERT_EQUALS(event_index.size(), 1439);
    TS_ASSERT_THROWS_EQUALS(
        auto event_id = Nioh::readNexusVector<uint16_t>(file, "event_id"),
        std::runtime_error & e, std::string(e.what()),
        "Downcasting is forbidden in NeXusIOHelper::readNexusAnyVector");
    TS_ASSERT_THROWS_EQUALS(
        auto event_time_offset =
            Nioh::readNexusVector<uint16_t>(file, "event_time_offset"),
        std::runtime_error & e, std::string(e.what()),
        "Downcasting is forbidden in NeXusIOHelper::readNexusAnyVector");
    TS_ASSERT_THROWS_EQUALS(
        auto event_time_zero =
            Nioh::readNexusVector<float>(file, "event_time_zero"),
        std::runtime_error & e, std::string(e.what()),
        "Downcasting is forbidden in NeXusIOHelper::readNexusAnyVector");
    file.closeGroup();
    file.closeGroup();
  }

  void test_nexus_io_helper_readNexusVector_v20_ess_integration_2018() {
    const std::string filename =
        Mantid::API::FileFinder::Instance().getFullPath(
            "V20_ESSIntegration_2018-12-13_0942.nxs");
    ::NeXus::File file(filename);
    file.openGroup("entry", "NXentry");
    file.openGroup("event_data", "NXevent_data");
    auto event_index = Nioh::readNexusVector<uint64_t>(file, "event_index");
    TS_ASSERT_EQUALS(event_index.size(), 38258);
    auto event_id = Nioh::readNexusVector<uint64_t>(file, "event_id");
    TS_ASSERT_EQUALS(event_id.size(), 43277);
    auto event_time_offset =
        Nioh::readNexusVector<float>(file, "event_time_offset");
    TS_ASSERT_EQUALS(event_time_offset.size(), 43277);
    auto event_time_zero =
        Nioh::readNexusVector<double>(file, "event_time_zero");
    TS_ASSERT_EQUALS(event_time_zero.size(), 38258);
    file.closeGroup();
    file.closeGroup();
  }

  void test_nexus_io_helper_readNexusSlab() {
    const std::string filename =
        Mantid::API::FileFinder::Instance().getFullPath("V20_ESS_example.nxs");
    ::NeXus::File file(filename);
    file.openGroup("entry", "NXentry");
    file.openGroup("raw_event_data", "NXevent_data");
    auto event_index =
        Nioh::readNexusSlab<uint64_t>(file, "event_index", {10}, {200});
    TS_ASSERT_EQUALS(event_index.size(), 200);
    auto event_id =
        Nioh::readNexusSlab<uint64_t>(file, "event_id", {100}, {300});
    TS_ASSERT_EQUALS(event_id.size(), 300);
    auto event_time_offset =
        Nioh::readNexusSlab<float>(file, "event_time_offset", {1000}, {400});
    TS_ASSERT_EQUALS(event_time_offset.size(), 400);
    auto event_time_zero =
        Nioh::readNexusSlab<double>(file, "event_time_zero", {111}, {501});
    TS_ASSERT_EQUALS(event_time_zero.size(), 501);
    file.closeGroup();
    file.closeGroup();
  }

  void test_nexus_io_helper_readNexusSlab_throws_when_downcasting() {
    const std::string filename =
        Mantid::API::FileFinder::Instance().getFullPath("V20_ESS_example.nxs");
    ::NeXus::File file(filename);
    file.openGroup("entry", "NXentry");
    file.openGroup("raw_event_data", "NXevent_data");
    auto event_index =
        Nioh::readNexusSlab<uint64_t>(file, "event_index", {111}, {222});
    TS_ASSERT_EQUALS(event_index.size(), 222);
    TS_ASSERT_THROWS_EQUALS(
        auto event_id =
            Nioh::readNexusSlab<uint16_t>(file, "event_id", {222}, {333}),
        std::runtime_error & e, std::string(e.what()),
        "Downcasting is forbidden in NeXusIOHelper::readNexusAnySlab");
    TS_ASSERT_THROWS_EQUALS(
        auto event_time_offset = Nioh::readNexusSlab<uint16_t>(
            file, "event_time_offset", {333}, {444}),
        std::runtime_error & e, std::string(e.what()),
        "Downcasting is forbidden in NeXusIOHelper::readNexusAnySlab");
    TS_ASSERT_THROWS_EQUALS(
        auto event_time_zero =
            Nioh::readNexusSlab<float>(file, "event_time_zero", {444}, {555}),
        std::runtime_error & e, std::string(e.what()),
        "Downcasting is forbidden in NeXusIOHelper::readNexusAnySlab");
    file.closeGroup();
    file.closeGroup();
  }

  void test_nexus_io_helper_readNexusSlab_v20_ess_integration_2018() {
    const std::string filename =
        Mantid::API::FileFinder::Instance().getFullPath(
            "V20_ESSIntegration_2018-12-13_0942.nxs");
    ::NeXus::File file(filename);
    file.openGroup("entry", "NXentry");
    file.openGroup("event_data", "NXevent_data");
    auto event_index =
        Nioh::readNexusSlab<uint64_t>(file, "event_index", {111}, {222});
    TS_ASSERT_EQUALS(event_index.size(), 222);
    auto event_id =
        Nioh::readNexusSlab<uint64_t>(file, "event_id", {333}, {444});
    TS_ASSERT_EQUALS(event_id.size(), 444);
    auto event_time_offset =
        Nioh::readNexusSlab<float>(file, "event_time_offset", {555}, {666});
    TS_ASSERT_EQUALS(event_time_offset.size(), 666);
    auto event_time_zero =
        Nioh::readNexusSlab<double>(file, "event_time_zero", {777}, {888});
    TS_ASSERT_EQUALS(event_time_zero.size(), 888);
    file.closeGroup();
    file.closeGroup();
  }
};

#endif /*NEXUSIOHELPERTEST_H_*/
