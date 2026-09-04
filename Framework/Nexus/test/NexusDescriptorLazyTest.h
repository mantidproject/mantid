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

  /// Regression guard against a use-after-move in operator[]: it once did
  /// `m_allEntries[name] = std::move(nxclass); return nxclass;`, so the FIRST (uncached, disk-path)
  /// lookup of an entry returned the moved-from empty string. That made the first openGroup NX_class
  /// validation of an uncached group spuriously fail ("... does not have class NXnote"), breaking loads.
  void test_operator_index_first_call_returns_real_class() {
    std::cout << "\nTesting operator[] first-call (disk path) in NexusDescriptorLazy" << std::endl;
    std::string const filename = NexusTest::getFullPath("EQSANS_89157.nxs.h5");
    // These sit at depth 3, below the bounded init scan, so the FIRST query takes the disk branch.
    std::string const uncachedGroup = "/entry/DASlogs/BL6:CS:DataType"; // NX_class == NXlog
    std::string const uncachedData = "/entry/user1/facility_user_id";   // a dataset -> "SDS"

    { // group: the very first call must be the real class, not empty/moved-from
      Mantid::Nexus::NexusDescriptorLazy descriptor(filename);
      std::string const firstCall = descriptor[uncachedGroup];
      TS_ASSERT_EQUALS(firstCall, "NXlog");
      TS_ASSERT_EQUALS(descriptor[uncachedGroup], firstCall); // idempotent (now cached)
    }
    { // dataset: first call must report SDS
      Mantid::Nexus::NexusDescriptorLazy descriptor(filename);
      TS_ASSERT_EQUALS(descriptor[uncachedData], "SDS");
    }
    // the class-checking helpers route through operator[]; they must be correct on the first call
    {
      Mantid::Nexus::NexusDescriptorLazy descriptor(filename);
      TS_ASSERT_EQUALS(descriptor.isEntry(uncachedGroup, "NXlog"), true);
    }
    {
      Mantid::Nexus::NexusDescriptorLazy descriptor(filename);
      TS_ASSERT_EQUALS(descriptor.isDataSet(uncachedData), true);
    }
  }

  void test_classTypeExistsInCache() {
    std::cout << "\nTesting classTypeExistsInCache in NexusDescriptorLazy" << std::endl;
    std::string const filename = NexusTest::getFullPath("EQSANS_89157.nxs.h5");
    Mantid::Nexus::NexusDescriptorLazy descriptor(filename);

    // NXentry (root-level) and NXevent_data (direct child of /entry) are both within
    // the bounded init scan, so they must be found without walking the file.
    TS_ASSERT_EQUALS(descriptor.classTypeExistsInCache("NXentry"), true);
    TS_ASSERT_EQUALS(descriptor.classTypeExistsInCache("NXevent_data"), true);

    // a class that appears nowhere in the file cannot be in the cache either.
    TS_ASSERT_EQUALS(descriptor.classTypeExistsInCache("NXthisClassDoesNotExist"), false);
  }

  void test_allAddressesOfType() {
    std::cout << "\nTesting allAddressesOfType in NexusDescriptorLazy" << std::endl;
    std::string const filename = NexusTest::getFullPath("EQSANS_89157.nxs.h5");
    Mantid::Nexus::NexusDescriptorLazy descriptor(filename);

    // /entry/DASlogs/BL6:CS:DataType is NXlog, but sits below the bounded init scan,
    // so this call must trigger (and cache) the one-time full-file scan.
    auto const addresses = descriptor.allAddressesOfType("NXlog");
    TS_ASSERT(addresses.count("/entry/DASlogs/BL6:CS:DataType") == 1);

    // a repeat call must be served from the now-memoized cache and return the same result
    auto const addressesAgain = descriptor.allAddressesOfType("NXlog");
    TS_ASSERT_EQUALS(addresses, addressesAgain);
  }

  void test_classTypeExistsChild() {
    std::cout << "\nTesting classTypeExistsChild in NexusDescriptorLazy" << std::endl;
    std::string const filename = NexusTest::getFullPath("EQSANS_89157.nxs.h5");
    Mantid::Nexus::NexusDescriptorLazy descriptor(filename);

    // matching: /entry/DASlogs has a NXlog descendant
    TS_ASSERT_EQUALS(descriptor.classTypeExistsChild("/entry/DASlogs", "NXlog"), true);
    // non-matching: /entry/DASlogs has no NXevent_data descendant
    TS_ASSERT_EQUALS(descriptor.classTypeExistsChild("/entry/DASlogs", "NXevent_data"), false);
    // absent parent: the child cannot exist if the parent does not
    TS_ASSERT_EQUALS(descriptor.classTypeExistsChild("/entry/not_a_real_group", "NXlog"), false);
  }

  void test_registerEntry_and_registerDataSet() {
    std::cout << "\nTesting registerEntry/registerDataSet in NexusDescriptorLazy" << std::endl;
    std::string const filename = NexusTest::getFullPath("EQSANS_89157.nxs.h5");
    Mantid::Nexus::NexusDescriptorLazy descriptor(filename);

    std::string const newGroup = "/entry/a_newly_made_group";
    std::string const newData = "/entry/a_newly_made_dataset";

    // query first so the address is cached as a miss, then verify registration overrides it
    TS_ASSERT_EQUALS(descriptor.isEntry(newGroup), false);
    descriptor.registerEntry(newGroup, "NXcollection");
    TS_ASSERT_EQUALS(descriptor.isEntry(newGroup), true);
    TS_ASSERT_EQUALS(descriptor.isEntry(newGroup, "NXcollection"), true);

    TS_ASSERT_EQUALS(descriptor.isDataSet(newData), false);
    descriptor.registerDataSet(newData);
    TS_ASSERT_EQUALS(descriptor.isDataSet(newData), true);
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

    std::atomic<int> failureCount{0}; // threadsafe count of issues
    std::vector<std::thread> threads(NUM_THREAD);
    for (int i = 0; i < NUM_THREAD; ++i) {
      threads[i] = std::thread([&descriptor, &failureCount]() {
        // things that are always there
        if (!descriptor.hasRootAttr("file_name"))
          failureCount++;
        if (!descriptor.hasRootAttr("file_time"))
          failureCount++;
        if (!descriptor.isEntry("/entry", "NXentry"))
          failureCount++;
        if (!descriptor.isEntry("/entry/instrument"))
          failureCount++;
        if (!descriptor.isEntry("/entry/DASlogs"))
          failureCount++;
        if (!descriptor.isEntry("/entry/DASlogs/LambdaRequest"))
          failureCount++;
        // things that are never there
        if (descriptor.hasRootAttr("file_zaniness"))
          failureCount++;
        if (descriptor.isEntry("/entry/pants"))
          failureCount++;
        // things that are lazy loaded
        if (!descriptor.isEntry("/entry/instrument/chopper1/phase"))
          failureCount++;
      });
    }

    for (int i = 0; i < NUM_THREAD; ++i) {
      threads[i].join();
    }

    TS_ASSERT_EQUALS(failureCount.load(), 0);
  }
};
