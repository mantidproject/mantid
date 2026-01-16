// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/ConfigService.h"
#include "MantidNexus/NexusAddress.h"

#include <filesystem>

#include <cstddef> // std::size_t

#include <cxxtest/TestSuite.h>

using Mantid::Nexus::NexusAddress;

class NexusAddressTest : public CxxTest::TestSuite {

public:
  void test_construct_is_root() {
    NexusAddress np;
    TS_ASSERT_EQUALS(np.string(), "/");
  }

  void test_construct_copy() {
    NexusAddress np1("/entry1");
    NexusAddress np2(np1);
    TS_ASSERT_EQUALS(np2.string(), np1.string());
  }

  void test_construct_from_filepath() {
    std::filesystem::path p("/path/good");
    NexusAddress np(p);
    TS_ASSERT_EQUALS(np.string(), p.string());
  }

  void test_construct_from_filepath_lexically_normal() {
    std::filesystem::path p("/path/good/../other/");
    NexusAddress np(p);
    TS_ASSERT_EQUALS(np.string(), "/path/other");
  }

  void test_construct_from_string() {
    std::string p("/path/good");
    NexusAddress np(p);
    TS_ASSERT_EQUALS(np.string(), p);
  }

  void test_construct_from_string_lexically_normal() {
    std::string p("/path/good/../other/");
    NexusAddress np(p);
    TS_ASSERT_EQUALS(np.string(), "/path/other");
  }

  void test_assignment_operator_path() {
    NexusAddress np1("/entry");
    NexusAddress np2("/other");
    TS_ASSERT_DIFFERS(np1.string(), np2.string())
    np2 = np1;
    TS_ASSERT_EQUALS(np1.string(), np2.string());
  }

  void test_assignment_operator_string() {
    NexusAddress np("/entry");
    std::string str("/other");
    TS_ASSERT_DIFFERS(np.string(), str)
    np = str;
    TS_ASSERT_EQUALS(np.string(), str);
  }

  void test_comparisons() {
    std::string str1("/entry"), str2("/entry"), str3("/other");
    NexusAddress np1(str1), np2(str2), np3(str3);
    // comparison with NexusAddress
    TS_ASSERT_EQUALS((np1 == np2), true);
    TS_ASSERT_EQUALS((np1 != np2), false);
    TS_ASSERT_EQUALS((np1 == np3), false);
    TS_ASSERT_EQUALS((np1 != np3), true);
    // comparison with string
    TS_ASSERT_EQUALS((np1 == str1), true);
    TS_ASSERT_EQUALS((np1 != str1), false);
    TS_ASSERT_EQUALS((np1 == str2), true);
    TS_ASSERT_EQUALS((np1 != str2), false);
    TS_ASSERT_EQUALS((np1 == str3), false);
    TS_ASSERT_EQUALS((np1 != str3), true);
    // swap comparison order
    TS_ASSERT_EQUALS((str1 == np1), true);
    TS_ASSERT_EQUALS((str1 != np1), false);
    // TS ASSERTS
    TS_ASSERT_EQUALS(np1, np2);
    TS_ASSERT_DIFFERS(np1, np3);
    TS_ASSERT_EQUALS(np1, str2);
    TS_ASSERT_DIFFERS(np1, str3);
  }

  void test_append() {
    NexusAddress start("/entry"), next("another");
    std::string another("one_more");

    // operator /
    NexusAddress up1 = start / next;
    TS_ASSERT_EQUALS(up1, "/entry/another");

    NexusAddress up2 = start / another;
    TS_ASSERT_EQUALS(up2, "/entry/one_more");

    // operator /=
    start /= another;
    TS_ASSERT_EQUALS(start, up2);
  }

  void test_isAbsolute() {
    NexusAddress abs("/entry/data1"), notabs("data2/something");
    TS_ASSERT_EQUALS(abs.isAbsolute(), true);
    TS_ASSERT_EQUALS(notabs.isAbsolute(), false);
  }

  void test_isRoot() {
    NexusAddress root1, root2("/"), notroot("/entry1");
    TS_ASSERT_EQUALS(NexusAddress::root().isRoot(), true);
    TS_ASSERT_EQUALS(root1.isRoot(), true);
    TS_ASSERT_EQUALS(root2.isRoot(), true);
    TS_ASSERT_EQUALS(NexusAddress::root().isAbsolute(), true);
    TS_ASSERT_EQUALS(root1.isAbsolute(), true);
    TS_ASSERT_EQUALS(root2.isAbsolute(), true);
    // not root
    TS_ASSERT_EQUALS(notroot.isRoot(), false);
  }

  void test_hasChild() {
    NexusAddress start("/entry"), next("another");
    std::string another("one_more");

    // operator /
    NexusAddress up1 = start / next;
    TS_ASSERT_EQUALS(up1, "/entry/another");
    TS_ASSERT(start.hasChild(up1))

    NexusAddress up2 = start / another;
    TS_ASSERT_EQUALS(up2, "/entry/one_more");
    TS_ASSERT(start.hasChild(up2))
  }

  void test_parent_path() {
    NexusAddress root;
    TS_ASSERT_EQUALS(root.parent_path(), root);

    std::filesystem::path path("/entry1/data_points/logs/log_values");
    NexusAddress long_path(path);
    TS_ASSERT_EQUALS(long_path, path.string());

    long_path = long_path.parent_path();
    path = path.parent_path();
    TS_ASSERT_EQUALS(long_path, path.string());
    TS_ASSERT_EQUALS(long_path, "/entry1/data_points/logs");

    long_path = long_path.parent_path();
    path = path.parent_path();
    TS_ASSERT_EQUALS(long_path, path.string());
    TS_ASSERT_EQUALS(long_path, "/entry1/data_points");

    long_path = long_path.parent_path();
    path = path.parent_path();
    TS_ASSERT_EQUALS(long_path, path.string());
    TS_ASSERT_EQUALS(long_path, "/entry1");

    long_path = long_path.parent_path();
    path = path.parent_path();
    TS_ASSERT_EQUALS(long_path, path.string());
    TS_ASSERT_EQUALS(long_path, "/");

    long_path = long_path.parent_path();
    path = path.parent_path();
    TS_ASSERT_EQUALS(long_path, path.string());
    TS_ASSERT_EQUALS(long_path, "/");
  }

  void test_fromRoot() {
    NexusAddress np("entry2/data");
    NexusAddress npabs = np.fromRoot();
    TS_ASSERT_EQUALS(np.isAbsolute(), false);
    TS_ASSERT_EQUALS(npabs.isAbsolute(), true);
    TS_ASSERT_EQUALS(npabs, "/" + np.string());
    TS_ASSERT_EQUALS(npabs.fromRoot(), npabs);
  }

  void test_stem() {
    NexusAddress root;
    TS_ASSERT_EQUALS(root.stem(), "");

    NexusAddress long_path("/entry1/data_points/logs/log_values");
    TS_ASSERT_EQUALS(long_path.stem(), "log_values");
  }

  void test_root() {
    NexusAddress root;
    TS_ASSERT_EQUALS(root.root(), root);
    TS_ASSERT_EQUALS(root.root(), NexusAddress::root());
    TS_ASSERT_EQUALS(root.root(), "/");

    NexusAddress long_path("/entry1/data_points/logs/log_values");
    TS_ASSERT_EQUALS(long_path.root(), root);
    TS_ASSERT_EQUALS(long_path.root(), NexusAddress::root());
    TS_ASSERT_EQUALS(long_path.root(), "/");
  }

  void test_parts() {
    std::vector<std::string> names{"one", "two", "three", "four"};
    NexusAddress np;
    for (auto name : names) {
      np /= name;
    }
    TS_ASSERT_EQUALS(np.string(), "/one/two/three/four");
    std::vector<std::string> actual(np.parts());
    for (std::size_t i = 0; i < actual.size(); i++) {
      TS_ASSERT_EQUALS(names[i], actual[i]);
    }

    NexusAddress np2("/notroot");
    std::vector<std::string> part(np2.parts());
    TS_ASSERT_EQUALS(part.size(), 1);
    TS_ASSERT_EQUALS(part[0], "notroot")
  }

  void test_string_concat() {
    NexusAddress np("/entry1/two");
    std::string pref = "path located at ";
    std::string post = " is a good path";
    char carr[] = " a c style string ";

    TS_ASSERT_EQUALS("path located at /entry1/two", pref + np);
    TS_ASSERT_EQUALS("/entry1/two is a good path", np + post);
    TS_ASSERT_EQUALS(" a c style string /entry1/two", carr + np);
    TS_ASSERT_EQUALS("/entry1/two a c style string ", np + carr);
  }

  std::string function_with_string_argument(std::string x) {
    std::stringstream msg;
    msg << "Writing out string " << x << "\n";
    return msg.str();
  }

  void test_nexuspath_as_string_argument() {
    NexusAddress np("/entry1/two");
    std::string out;
    TS_ASSERT_THROWS_NOTHING(out = function_with_string_argument(np));
    TS_ASSERT_EQUALS(out, "Writing out string /entry1/two\n");
  }

  void test_c_str() {
    NexusAddress np("/entry/data/comp_data");
    std::string out(np.c_str());
    TS_ASSERT_EQUALS(out, np.string());
  }

  void test_root_root() {
    NexusAddress np("//raw_data_1");
    TS_ASSERT_EQUALS(np, NexusAddress("/raw_data_1"));
    TS_ASSERT_EQUALS(np.string(), "/raw_data_1");

    std::filesystem::path p("//raw_data_1");
    NexusAddress np2(p);
    TS_ASSERT_EQUALS(np2.string(), "/raw_data_1");
  }

  void test_abs_slash_abs() {
    NexusAddress np("/entry0");
    std::string name("/data");
    TS_ASSERT_EQUALS((np / name).string(), "/entry0/data");

    NexusAddress np2("/data");
    TS_ASSERT_EQUALS((np / np2).string(), "/entry0/data");
  }

  void test_slash() {
    NexusAddress np("/entry0");
    std::string name("data/copy");
    TS_ASSERT_EQUALS((np / name).string(), "/entry0/data/copy");

    NexusAddress np2("data/copy");
    TS_ASSERT_EQUALS((np / np2).string(), "/entry0/data/copy");
  }
};
