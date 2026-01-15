// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidNexus/NexusDescriptorLazy.h"
#include "test_helper.h"

#include <filesystem>
#include <fstream>
#include <thread>

#include <cxxtest/TestSuite.h>

using Mantid::Nexus::NexusDescriptorLazy;

class NexusDescriptorLazyTest : public CxxTest::TestSuite {

public:
  void test_fails_bad_file() {
    std::cout << "\nTesting bad file handling in NexusDescriptorLazy" << std::endl;
    // test opening a file that exists, but is unreadable
    std::string filename = NexusTest::getFullPath("Test_characterizations_char.txt");
    TS_ASSERT_THROWS(Mantid::Nexus::NexusDescriptorLazy nd(filename), std::invalid_argument const &);

    filename = "fake_empty_file.nxs.h5";
    std::ofstream file(filename);
    file << "mock";
    file.close();
    TS_ASSERT_THROWS(Mantid::Nexus::NexusDescriptorLazy nd(filename), std::invalid_argument const &);
    std::filesystem::remove(filename);
  }

  void test_extension() {
    std::cout << "\nTesting extension retrieval in NexusDescriptorLazy" << std::endl;
    std::string const filename = NexusTest::getFullPath("EQSANS_89157.nxs.h5");
    Mantid::Nexus::NexusDescriptorLazy descriptor(filename);
    TS_ASSERT_EQUALS(descriptor.extension(), ".h5");
  }

  void test_filename() {
    std::cout << "\nTesting filename retrieval in NexusDescriptorLazy" << std::endl;
    std::string const filename = NexusTest::getFullPath("EQSANS_89157.nxs.h5");
    Mantid::Nexus::NexusDescriptorLazy descriptor(filename);
    TS_ASSERT_EQUALS(descriptor.filename(), filename);
  }

  void test_init_loads() {
    std::cout << "\nTesting initialization in NexusDescriptorLazy" << std::endl;
    // create a descriptor with the correct values
    std::string const filename = NexusTest::getFullPath("EQSANS_89157.nxs.h5");
    Mantid::Nexus::NexusDescriptorLazy descriptor(filename);

    auto entries = descriptor.getAllEntries();

    // verify that entries were loaded
    TS_ASSERT_EQUALS(entries.count("/entry"), 1);
    TS_ASSERT_EQUALS(entries.count("/entry/instrument"), 1);
    TS_ASSERT_EQUALS(entries.count("/entry/instrument/bank39/total_counts"), 1);

    // verify entries have correct classes
    TS_ASSERT_EQUALS(entries["/entry"], "NXentry");
    TS_ASSERT_EQUALS(entries["/entry/instrument"], "NXinstrument");
    TS_ASSERT_EQUALS(entries["/entry/instrument/bank39/total_counts"], "SDS");

    // verify that non-existing groups are not there
    TS_ASSERT_EQUALS(entries.count("/entry/shorts"), 0);
    TS_ASSERT_EQUALS(entries.count("/entry/instrument/pants"), 0);
  }

  void test_firstEntryNameType() {
    std::cout << "\nTesting firstEntryNameType in NexusDescriptorLazy" << std::endl;
    // create a descriptor with the correct values
    std::string const filename = NexusTest::getFullPath("EQSANS_89157.nxs.h5");
    Mantid::Nexus::NexusDescriptorLazy descriptor(filename);
    const auto &firstEntry = descriptor.firstEntryNameType();
    TS_ASSERT_EQUALS(firstEntry.first, "entry");
    TS_ASSERT_EQUALS(firstEntry.second, "NXentry");
  }

  void test_isEntry() {
    std::cout << "\nTesting isEntry in NexusDescriptorLazy" << std::endl;
    // create a descriptor with the correct values
    std::string const filename = NexusTest::getFullPath("EQSANS_89157.nxs.h5");
    Mantid::Nexus::NexusDescriptorLazy descriptor(filename);

    // verify that existing groups are there
    TS_ASSERT_EQUALS(descriptor.isEntry("/entry/DASlogs"), true);
    TS_ASSERT_EQUALS(descriptor.isEntry("/entry/user1/facility_user_id"), true);
    TS_ASSERT_EQUALS(descriptor.isEntry("/entry/instrument/bank39"), true);
    TS_ASSERT_EQUALS(descriptor.isEntry("/entry/instrument/bank39/total_counts"), true);

    // verify that non-existing groups are not there
    TS_ASSERT_EQUALS(descriptor.isEntry("/entry/shorts"), false);
    TS_ASSERT_EQUALS(descriptor.isEntry("/entry/instrument/pants"), false);
  }

  void test_hasRootAttr() {
    std::cout << "\nTesting hasRootAttr in NexusDescriptorLazy" << std::endl;
    // create a descriptor with the correct values
    const std::string filename = NexusTest::getFullPath("EQSANS_89157.nxs.h5");
    Mantid::Nexus::NexusDescriptorLazy descriptor(filename);

    // verify that existing root attributes are there
    TS_ASSERT_EQUALS(descriptor.hasRootAttr("file_name"), true);
    TS_ASSERT_EQUALS(descriptor.hasRootAttr("file_time"), true);

    // verify that non-existing root attributes are not there
    TS_ASSERT_EQUALS(descriptor.hasRootAttr("not_an_attr"), false);
  }

  void test_getStrData() {
    std::cout << "\nTesting getStrData in NexusDescriptorLazy" << std::endl;
    // create a descriptor with the correct values
    const std::string filename = NexusTest::getFullPath("EQSANS_89157.nxs.h5");
    Mantid::Nexus::NexusDescriptorLazy descriptor(filename);

    // verify that existing data can be read
    TS_ASSERT_EQUALS(descriptor.getStrData("/entry/instrument/name"), "EQ-SANS");

    // verify that non-existing data returns empty string
    TS_ASSERT_EQUALS(descriptor.getStrData("/entry/instrument/not_a_data"), "");
  }

  void test_init_loads_class() {
    std::cout << "\nTesting classTypeExists in NexusDescriptorLazy" << std::endl;
    // create a descriptor with the correct values
    std::string const filename = NexusTest::getFullPath("HB3A_data.nxs");
    Mantid::Nexus::NexusDescriptorLazy descriptor(filename);
    // verify that class types are correctly identified
    TS_ASSERT_EQUALS(descriptor.classTypeExists("NXentry"), true);
    TS_ASSERT(descriptor.isEntry("/MDHistoWorkspace"));
    TS_ASSERT(descriptor.isEntry("/MDHistoWorkspace", "NXentry"));
  }

  void test_threadSafety() {
    constexpr int NUM_THREAD{5}; // number of threads to spawn

    const std::string filename = NexusTest::getFullPath("EQSANS_89157.nxs.h5");
    Mantid::Nexus::NexusDescriptorLazy descriptor(filename);

    std::vector<std::thread> threads(NUM_THREAD);
    for (int i = 0; i < NUM_THREAD; ++i) {
      threads[i] = std::thread([&descriptor]() {
        TS_ASSERT_EQUALS(descriptor.hasRootAttr("file_name"), true);
        TS_ASSERT_EQUALS(descriptor.hasRootAttr("file_time"), true);
        TS_ASSERT(descriptor.isEntry("/entry", "NXentry"));
        TS_ASSERT_EQUALS(descriptor.isEntry("/entry/instrument"), true);
        TS_ASSERT_EQUALS(descriptor.isEntry("/entry/DASlogs"), true);
        TS_ASSERT_EQUALS(descriptor.isEntry("/entry/DASlogs/LambdaRequest"), true);
      });
    }

    for (int i = 0; i < NUM_THREAD; ++i) {
      threads[i].join();
    }
  }
};
