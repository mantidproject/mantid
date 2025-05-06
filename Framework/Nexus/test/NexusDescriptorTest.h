// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/ConfigService.h"
#include "MantidNexus/NexusDescriptor.h"

#include <filesystem>

#include <cstddef> // std::size_t

#include <cxxtest/TestSuite.h>

using Mantid::Nexus::NexusDescriptor;

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

class NexusDescriptorTest : public CxxTest::TestSuite {

public:
  // test get functions getFilename and getAllEntries
  void test_nexus_hdf5_descriptor_get() {

    const std::string filename = getFullPath("EQSANS_89157.nxs.h5");

    Mantid::Nexus::NexusDescriptor nexusHDF5Descriptor(filename);

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

  void test_AddEntry() {
    // create a descriptor with the correct values
    const std::string filename = getFullPath("EQSANS_89157.nxs.h5");
    Mantid::Nexus::NexusDescriptor nexusHDF5Descriptor(filename);

    // verify that existing groups are there
    TS_ASSERT_EQUALS(nexusHDF5Descriptor.isEntry("/entry/DASlogs", "NXcollection"), true);
    TS_ASSERT_EQUALS(nexusHDF5Descriptor.isEntry("/entry/DASlogs/LambdaRequest", "NXlog"), true);
    TS_ASSERT_EQUALS(nexusHDF5Descriptor.isEntry("/entry/DASlogs/OmikronRequest", "NXlog"), false);

    // can't add a value with relative path
    TS_ASSERT_THROWS(nexusHDF5Descriptor.addEntry("entry/DASlogs/OmikronRequest", "NXlog"), const std::runtime_error &);
    TS_ASSERT_EQUALS(nexusHDF5Descriptor.isEntry("/entry/DASlogs/OmikronRequest", "NXlog"), false);

    // make sure you can't add a group with invalid parent
    TS_ASSERT_THROWS(nexusHDF5Descriptor.addEntry("/entry/DASlogginator/OmikronRequest", "NXlog"),
                     const std::runtime_error &);
    TS_ASSERT_EQUALS(nexusHDF5Descriptor.isEntry("/entry/DASlogginator/OmikronRequest", "NXlog"), false);

    // add a field correctly
    TS_ASSERT_THROWS_NOTHING(nexusHDF5Descriptor.addEntry("/entry/DASlogs/OmikronRequest", "NXlog"));
    TS_ASSERT_EQUALS(nexusHDF5Descriptor.isEntry("/entry/DASlogs/OmikronRequest", "NXlog"), true);
  }
};
