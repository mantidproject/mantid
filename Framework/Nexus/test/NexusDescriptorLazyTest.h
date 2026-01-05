// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/ConfigService.h"
#include "MantidNexus/NexusDescriptorLazy.h"
#include "test_helper.h"

#include <filesystem>
#include <fstream>

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
};
