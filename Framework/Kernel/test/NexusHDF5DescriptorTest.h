// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/ConfigService.h"
#include "MantidKernel/NexusHDF5Descriptor.h"

#include "Poco/File.h"
#include "Poco/Path.h"

#include <cstddef> // std::size_t

#include <cxxtest/TestSuite.h>

namespace {
std::string getFullPath(const std::string &filename) {
  using Mantid::Kernel::ConfigService;
  auto dataPaths = ConfigService::Instance().getDataSearchDirs();
  for (auto &dataPath : dataPaths) {
    Poco::Path hdf5Path(dataPath, filename);
    if (Poco::File(hdf5Path).exists()) {
      return hdf5Path.toString();
    }
  }
  return std::string();
}
} // namespace

class NexusHDF5DescriptorTest : public CxxTest::TestSuite {

public:
  // test get functions getFilename and getAllEntries
  void test_nexus_hdf5_descriptor_get() {

    const std::string filename = getFullPath("EQSANS_89157.nxs.h5");

    Mantid::Kernel::NexusHDF5Descriptor nexusHDF5Descriptor(filename);

    TS_ASSERT_EQUALS(filename, nexusHDF5Descriptor.getFilename());

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
  }
};
