// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/ConfigService.h"
#include "MantidNexus/NexusPath.h"

#include <filesystem>

#include <cstddef> // std::size_t

#include <cxxtest/TestSuite.h>

using NeXus::NexusPath;

class NexusPathTest : public CxxTest::TestSuite {

public:
  void test_construct_is_root() {
    NexusPath np;
    TS_ASSERT_EQUALS(np.string(), "/");
  }

  void test_construct_copy() {
    NexusPath np1("/entry1");
    NexusPath np2(np1);
    TS_ASSERT_EQUALS(np2.string(), np1.string());
  }

  void test_construct_from_filepath() {
    std::filesystem::path p("/path/good");
    NexusPath np(p);
    TS_ASSERT_EQUALS(np.string(), p.string());
  }

  void test_construct_from_filepath_lexically_normal() {
    std::filesystem::path p("/path/good/../other");
    NexusPath np(p);
    TS_ASSERT_EQUALS(np.string(), "/path/other");
  }

  void test_construct_from_string() {
    std::string p("/path/good");
    NexusPath np(p);
    TS_ASSERT_EQUALS(np.string(), p);
  }

  void test_construct_from_string_lexically_normal() {
    std::string p("/path/good/../other/");
    NexusPath np(p);
    TS_ASSERT_EQUALS(np.string(), "/path/other");
  }

  void test_assignment_operator_path() {
    NexusPath np1("/entry");
    NexusPath np2("/other");
    TS_ASSERT_DIFFERS(np1.string(), np2.string())
    np2 = np1;
    TS_ASSERT_EQUALS(np1.string(), np2.string());
  }

  void test_assignment_operator_string() {
    NexusPath np("/entry");
    std::string str("/other");
    TS_ASSERT_DIFFERS(np.string(), str)
    np = str;
    TS_ASSERT_EQUALS(np.string(), str);
  }

  void test_comparisons() {
    std::string str1("/entry"), str2("/entry"), str3("/other");
    NexusPath np1(str1), np2(str2), np3(str3);
    // comparison with NexusPath
    TS_ASSERT_EQUALS(np1 == np2, true);
    TS_ASSERT_EQUALS(np1 != np2, false);
    TS_ASSERT_EQUALS(np1 == np3, false);
    TS_ASSERT_EQUALS(np1 != np3, true);
    // comparison with string
    TS_ASSERT_EQUALS(np1 == str1, true);
    TS_ASSERT_EQUALS(np1 != str1, false);
    TS_ASSERT_EQUALS(np1 == str2, true);
    TS_ASSERT_EQUALS(np1 != str2, false);
    TS_ASSERT_EQUALS(np1 == str3, false);
    TS_ASSERT_EQUALS(np1 != str3, true);
    // swap comparison order
    TS_ASSERT_EQUALS(str1 == np1, true);
    TS_ASSERT_EQUALS(str1 != np1, false);
    // TS ASSERTS
    TS_ASSERT_EQUALS(np1, np2);
    TS_ASSERT_DIFFERS(np1, np3);
    TS_ASSERT_EQUALS(np1, str2);
    TS_ASSERT_DIFFERS(np1, str3);
  }

  void test_append() {
    NexusPath start("/entry"), next("another");
    std::string another("one_more");

    // operator /
    NexusPath up1 = start / next;
    TS_ASSERT_EQUALS(up1, "/entry/another");

    NexusPath up2 = start / another;
    TS_ASSERT_EQUALS(up2, "/entry/one_more");

    // operator /=
    start /= another;
    TS_ASSERT_EQUALS(start, up2);
  }

  void test_isAbsolute() {
    NexusPath abs("/entry/data1"), notabs("data2/something");
    TS_ASSERT_EQUALS(abs.isAbsolute(), true);
    TS_ASSERT_EQUALS(notabs.isAbsolute(), false);
  }

  void test_isRoot() {
    NexusPath root1, root2("/"), notroot("/entry1");
    TS_ASSERT_EQUALS(NexusPath::root().isRoot(), true);
    TS_ASSERT_EQUALS(root1.isRoot(), true);
    TS_ASSERT_EQUALS(root2.isRoot(), true);
    TS_ASSERT_EQUALS(NexusPath::root().isAbsolute(), true);
    TS_ASSERT_EQUALS(root1.isAbsolute(), true);
    TS_ASSERT_EQUALS(root2.isAbsolute(), true);
    // not root
    TS_ASSERT_EQUALS(notroot.isRoot(), false);
  }

  void test_parent_path() {
    NexusPath root;
    TS_ASSERT_EQUALS(root.parent_path(), root);

    std::filesystem::path path("/entry1/data_points/logs/log_values");
    NexusPath long_path(path);
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
    NexusPath np("entry2/data");
    NexusPath npabs = np.fromRoot();
    TS_ASSERT_EQUALS(np.isAbsolute(), false);
    TS_ASSERT_EQUALS(npabs.isAbsolute(), true);
    TS_ASSERT_EQUALS(npabs, "/" + np.string());
    TS_ASSERT_EQUALS(npabs.fromRoot(), npabs);
  }

  void test_stem() {
    NexusPath root;
    TS_ASSERT_EQUALS(root.stem(), "");

    NexusPath long_path("/entry1/data_points/logs/log_values");
    TS_ASSERT_EQUALS(long_path.stem(), "log_values");
  }

  void test_root() {
    NexusPath root;
    TS_ASSERT_EQUALS(root.root(), root);
    TS_ASSERT_EQUALS(root.root(), NexusPath::root());
    TS_ASSERT_EQUALS(root.root(), "/");

    NexusPath long_path("/entry1/data_points/logs/log_values");
    TS_ASSERT_EQUALS(long_path.root(), root);
    TS_ASSERT_EQUALS(long_path.root(), NexusPath::root());
    TS_ASSERT_EQUALS(long_path.root(), "/");
  }

  void test_string_concat() {
    NexusPath np("/entry1/two");
    std::string pref = "path located at ";
    std::string post = " is a good path";

    TS_ASSERT_EQUALS("path located at /entry1/two", pref + np);
    TS_ASSERT_EQUALS("/entry1/two is a good path", np + post);
  }

  std::string function_with_string_argument(std::string x) {
    std::stringstream msg;
    msg << "Writing out string " << x << "\n";
    return msg.str();
  }

  void test_nexuspath_as_string_argument() {
    NexusPath np("/entry1/two");
    std::string out;
    TS_ASSERT_THROWS_NOTHING(out = function_with_string_argument(np));
    TS_ASSERT_EQUALS(out, "Writing out string /entry1/two\n");
  }
};
