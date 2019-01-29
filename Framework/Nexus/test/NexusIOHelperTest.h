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

using namespace Mantid;
using namespace Mantid::NeXus::NeXusIOHelper;

class NexusIOHelperTest : public CxxTest::TestSuite {

public:
  void test_nexus_io_helper_readNexusVector() {
    const std::string filename =
        API::FileFinder::Instance().getFullPath("V20_ESS_example.nxs");
    ::NeXus::File file(filename);
    file.openGroup("entry", "NXentry");
    file.openGroup("raw_event_data", "NXevent_data");
    auto event_index = readNexusVector<uint64_t>(file, "event_index");
    TS_ASSERT_EQUALS(event_index.size(), 1439);
    auto event_id = readNexusVector<uint64_t>(file, "event_id");
    TS_ASSERT_EQUALS(event_index.size(), 1439);
    auto event_time_offset = readNexusVector<float>(file, "event_time_offset");
    TS_ASSERT_EQUALS(event_time_offset.size(), 1439);
    auto event_time_zero = readNexusVector<double>(file, "event_time_zero");
    TS_ASSERT_EQUALS(event_time_zero.size(), 1439);
    file.closeGroup();
    file.closeGroup();
  }

  void test_nexus_io_helper_readNexusVector_throws_when_downcasting() {
    const std::string filename =
        API::FileFinder::Instance().getFullPath("V20_ESS_example.nxs");
    ::NeXus::File file(filename);
    file.openGroup("entry", "NXentry");
    file.openGroup("raw_event_data", "NXevent_data");
    auto event_index = readNexusVector<uint64_t>(file, "event_index");
    TS_ASSERT_EQUALS(event_index.size(), 1439);
    TS_ASSERT_THROWS_EQUALS(
        auto event_id = readNexusVector<uint16_t>(file, "event_id"),
        std::runtime_error & e, std::string(e.what()),
        "Downcasting is forbidden in NeXusIOHelper::readNexusAnyVector");
    TS_ASSERT_THROWS_EQUALS(
        auto event_time_offset =
            readNexusVector<uint16_t>(file, "event_time_offset"),
        std::runtime_error & e, std::string(e.what()),
        "Downcasting is forbidden in NeXusIOHelper::readNexusAnyVector");
    TS_ASSERT_THROWS_EQUALS(
        auto event_time_zero = readNexusVector<float>(file, "event_time_zero"),
        std::runtime_error & e, std::string(e.what()),
        "Downcasting is forbidden in NeXusIOHelper::readNexusAnyVector");
    file.closeGroup();
    file.closeGroup();
  }
};

#endif /*NEXUSIOHELPERTEST_H_*/
