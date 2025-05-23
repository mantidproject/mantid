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

  void test_allPathsAtLevel() {

    typedef std::pair<std::string, std::string> Entry;
    typedef std::map<std::string, std::string> Entries;

    // setup a recursive group tree
    std::vector<Entry> tree{Entry{"/entry1", "NXentry"},
                            Entry{"/entry1/layer2a", "NXentry"},
                            Entry{"/entry1/layer2a/layer3a", "NXentry"},
                            Entry{"/entry1/layer2a/layer3b", "NXentry"},
                            Entry{"/entry1/layer2a/data1_vec_1", "SDS"},
                            Entry{"/entry1/layer2b", "NXentry"},
                            Entry{"/entry1/layer2b/layer3a", "NXentry"},
                            Entry{"/entry1/layer2b/layer3b", "NXentry"},
                            Entry{"/entry2", "NXentry"},
                            Entry{"/entry2/layer2c", "NXentry"},
                            Entry{"/entry2/layer2c/layer3c", "NXentry"}};

    std::string nonexistent("not_a_real_file.egg");
    Mantid::Nexus::NexusDescriptor nd(nonexistent);
    for (auto it = tree.begin(); it != tree.end(); it++) {
      nd.addEntry(it->first, it->second);
    }

    // at root level, should be entry1, entry2
    Entries actual = nd.allPathsAtLevel("/");
    Entries expected = {Entry{"entry1", "NXentry"}, Entry{"entry2", "NXentry"}};
    for (auto it = expected.begin(); it != expected.end(); it++) {
      TS_ASSERT_EQUALS(actual.count(it->first), 1);
      TS_ASSERT_EQUALS(it->second, actual[it->first]);
    }

    // within entry1, should be layer2a, layer2b
    actual = nd.allPathsAtLevel("/entry1");
    expected = Entries({Entry{"layer2a", "NXentry"}, Entry{"layer2b", "NXentry"}});
    for (auto it = expected.begin(); it != expected.end(); it++) {
      TS_ASSERT_EQUALS(actual.count(it->first), 1);
      TS_ASSERT_EQUALS(it->second, actual[it->first]);
    }

    // within entry1/layer2a, should be layer3a, layer3b, data1
    actual = nd.allPathsAtLevel("/entry1/layer2a");
    expected = Entries({Entry{"layer3a", "NXentry"}, Entry{"layer3b", "NXentry"}, Entry{"data1_vec_1", "SDS"}});
    for (auto it = expected.begin(); it != expected.end(); it++) {
      TS_ASSERT_EQUALS(actual.count(it->first), 1);
      TS_ASSERT_EQUALS(it->second, actual[it->first]);
    }

    // within entry2/layer2c, should be layer3c
    actual = nd.allPathsAtLevel("/entry2/layer2c");
    expected = Entries({Entry{"layer3c", "NXentry"}});
    for (auto it = expected.begin(); it != expected.end(); it++) {
      TS_ASSERT_EQUALS(actual.count(it->first), 1);
      TS_ASSERT_EQUALS(it->second, actual[it->first]);
    }
  }
};
