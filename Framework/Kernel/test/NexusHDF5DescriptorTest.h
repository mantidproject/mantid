// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/ConfigService.h"
#include "MantidKernel/NexusHDF5Descriptor.h"

#include <filesystem>

#include <cstddef> // std::size_t

#include <cxxtest/TestSuite.h>

using Mantid::Kernel::NexusHDF5Descriptor;

namespace {
std::string getFullPath(const std::string &filename) {
  using Mantid::Kernel::ConfigService;
  auto dataPaths = ConfigService::Instance().getDataSearchDirs();
  for (auto &dataPath : dataPaths) {
    const auto hdf5Path = std::filesystem::path(dataPath) / filename;
    if (std::filesystem::exists(hdf5Path)) {
      return hdf5Path.string();
    }
  }
  return std::string();
}
} // namespace

class NexusHDF5DescriptorTest : public CxxTest::TestSuite {

public:
  NexusHDF5DescriptorTest() {
    using Mantid::Kernel::ConfigService;
    auto dataPaths = ConfigService::Instance().getDataSearchDirs();
    for (auto &dataPath : dataPaths) {
      const auto hdf5Path = std::filesystem::path(dataPath) / "CNCS_7860_event.nxs";
      if (std::filesystem::exists(hdf5Path))
        m_testHDF5Path = hdf5Path.string();

      const auto hdf4Path = std::filesystem::path(dataPath) / "argus0026287.nxs";
      if (std::filesystem::exists(hdf4Path))
        m_testHDF4Path = hdf4Path.string();

      const auto nonhdf5Path = std::filesystem::path(dataPath) / "CSP79590.raw";
      if (std::filesystem::exists(nonhdf5Path))
        m_testNonHDFPath = nonhdf5Path.string();

      if (!m_testHDF5Path.empty() && !m_testHDF4Path.empty() && !m_testNonHDFPath.empty())
        break;
    }
    if (m_testHDF5Path.empty() || m_testHDF4Path.empty() || m_testNonHDFPath.empty()) {
      throw std::runtime_error("Unable to find test files for FileDescriptorTest. "
                               "The AutoTestData directory needs to be in the search path");
    }
  }

  // test get functions getFilename and getAllEntries
  void test_nexus_hdf5_descriptor_get() {

    const std::string filename = getFullPath("EQSANS_89157.nxs.h5");

    Mantid::Kernel::NexusHDF5Descriptor nexusHDF5Descriptor(filename);

    TS_ASSERT_EQUALS(filename, nexusHDF5Descriptor.filename());
    TS_ASSERT_EQUALS(".h5", nexusHDF5Descriptor.extension());

    TS_ASSERT_EQUALS(nexusHDF5Descriptor.isEntry("/entry/instrument/bank39/total_counts", "SDS"), true);

    TS_ASSERT_EQUALS(nexusHDF5Descriptor.isEntry("/entry/DASlogs"), true);

    const std::map<std::string, std::set<std::string>> &allEntries = nexusHDF5Descriptor.getAllEntries();

    TS_ASSERT_EQUALS(allEntries.size(), 12);

    // confirms existence of groupClass key and expectedSize for value set
    auto lf_TestSet = [&](const std::string &groupClass, const std::size_t expectedSize) -> std::size_t {
      auto itClass = allEntries.find(groupClass);
      TS_ASSERT_DIFFERS(itClass, allEntries.end());
      TS_ASSERT_EQUALS(itClass->second.size(), expectedSize);
      return expectedSize;
    };

    std::size_t nEntries = 0;
    nEntries += lf_TestSet("NXcollection", 39);
    nEntries += lf_TestSet("NXdetector", 48);
    nEntries += lf_TestSet("NXdisk_chopper", 4);
    nEntries += lf_TestSet("NXentry", 1);
    nEntries += lf_TestSet("NXevent_data", 48);
    nEntries += lf_TestSet("NXinstrument", 1);
    nEntries += lf_TestSet("NXlog", 204);
    nEntries += lf_TestSet("NXmonitor", 3);
    nEntries += lf_TestSet("NXnote", 1);
    nEntries += lf_TestSet("NXsample", 1);
    nEntries += lf_TestSet("NXuser", 6);
    nEntries += lf_TestSet("SDS", 2567);

    TS_ASSERT_EQUALS(nEntries, 2923);

    // test firstEntryNameType
    TS_ASSERT_EQUALS(nexusHDF5Descriptor.firstEntryNameType().first, "entry");
    TS_ASSERT_EQUALS(nexusHDF5Descriptor.firstEntryNameType().second, "NXentry");

    // test classTypeExists
    TS_ASSERT(nexusHDF5Descriptor.classTypeExists("NXentry"));
    TS_ASSERT(!nexusHDF5Descriptor.classTypeExists("NOT_TYPE"));

    // test allPathsOfType
    TS_ASSERT_EQUALS(nexusHDF5Descriptor.allPathsOfType("NXentry").size(), 1);
    TS_ASSERT_EQUALS(nexusHDF5Descriptor.allPathsOfType("NXmonitor").size(), 3);
    TS_ASSERT_EQUALS(nexusHDF5Descriptor.allPathsOfType("SDS").size(), 2567);

    // test hasRootAttr
    TS_ASSERT(nexusHDF5Descriptor.hasRootAttr("file_name"));
    TS_ASSERT(!nexusHDF5Descriptor.hasRootAttr("not_attr"));
  }

  //=================================== Static isReadable methods
  //======================================
  void test_isReadable_Returns_False_For_Non_HDF_Filename() {
    TS_ASSERT(!NexusHDF5Descriptor::isReadable(m_testNonHDFPath, NexusHDF5Descriptor::Version4));
    TS_ASSERT(!NexusHDF5Descriptor::isReadable(m_testNonHDFPath, NexusHDF5Descriptor::Version5));
  }

  void test_isReadable_With_Version4_Returns_True_Only_For_HDF4() {
    TS_ASSERT(NexusHDF5Descriptor::isReadable(m_testHDF4Path, NexusHDF5Descriptor::Version4));
    TS_ASSERT(!NexusHDF5Descriptor::isReadable(m_testHDF5Path, NexusHDF5Descriptor::Version4));
  }

  void test_isReadable_With_Version5_Returns_True_Only_For_HDF4() {
    TS_ASSERT(NexusHDF5Descriptor::isReadable(m_testHDF5Path, NexusHDF5Descriptor::Version5));
    TS_ASSERT(!NexusHDF5Descriptor::isReadable(m_testHDF4Path, NexusHDF5Descriptor::Version5));
  }

  void test_getHDFVersion() {
    TS_ASSERT_EQUALS(NexusHDF5Descriptor::Version5, NexusHDF5Descriptor::getHDFVersion(m_testHDF5Path));
    TS_ASSERT_EQUALS(NexusHDF5Descriptor::Version4, NexusHDF5Descriptor::getHDFVersion(m_testHDF4Path));
    TS_ASSERT_EQUALS(NexusHDF5Descriptor::None, NexusHDF5Descriptor::getHDFVersion(m_testNonHDFPath));
  }

  void test_isReadable_Throws_With_Invalid_Filename() {
    TS_ASSERT_THROWS(NexusHDF5Descriptor::isReadable(""), const std::invalid_argument &);
  }

private:
  std::string m_testHDF5Path;
  std::string m_testHDF4Path;
  std::string m_testNonHDFPath;
};
