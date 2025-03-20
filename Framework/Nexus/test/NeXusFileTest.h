// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidNexus/NeXusException.hpp"
#include "MantidNexus/NeXusFile.hpp"
#include "test_helper.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

using namespace NeXus;
using namespace NexusTest;
using std::cout;
using std::endl;
using std::map;
using std::multimap;
using std::string;
using std::vector;

class NeXusFileTest : public CxxTest::TestSuite {

public:
  // // This pair of boilerplate methods prevent the suite being created statically
  // // This means the constructor isn't called when running other tests
  static NeXusFileTest *createSuite() { return new NeXusFileTest(); }
  static void destroySuite(NeXusFileTest *suite) { delete suite; }

  void test_compile() {
    string x = strmakef("not_a_real_file_%d", 10);
    removeFile(x);
    TS_ASSERT(true);
  }

  void test_remove() {
    // create a simple file, and make sure removeFile works as intended
    cout << "\nremoving\n";
    string filename("not_a_real_file.txt");

    // ensure file doesn't already exist
    if (std::filesystem::exists(filename)) {
      std::filesystem::remove(filename);
    }
    TS_ASSERT(!std::filesystem::exists(filename));

    // removeFile works fine if file doesn't exist
    TS_ASSERT_THROWS_NOTHING(removeFile(filename));

    // create the file
    std::ofstream outfile{filename};
    outfile.close();
    TS_ASSERT(std::filesystem::exists(filename));

    // remove it, make sure removed
    removeFile(filename);
    TS_ASSERT(!std::filesystem::exists(filename));
  }

  void test_can_create() {
    cout << "\ntest creation\n";

    string filename("test_nexus_file_init.h5");
    removeFile(filename);

    // create the file and ensure it exists
    NeXus::File file(filename, NXACC_CREATE5);
    file.close();
    TS_ASSERT(std::filesystem::exists(filename));

    // cleanup
    removeFile(filename);
  }

  void test_flush() {
    cout << "\ntest flush\n";
    // make sure flush works
    string filename("test_nexus_file_flush.h5");
    NeXus::File file(filename, NXACC_CREATE5);
    TS_ASSERT_THROWS_NOTHING(file.flush());

    // cleanup
    file.close();
    removeFile(filename);
  }

  void test_make_group() {
    cout << "\ntest makeGroup\n";
    string filename("test_nexus_file_grp.h5");
    removeFile(filename);
    NeXus::File file(filename, NXACC_CREATE5);

    string grp("test_group"), cls("NXsample");

    // check error conditions
    TS_ASSERT_THROWS(file.makeGroup(grp, ""), NeXus::Exception &);
    TS_ASSERT_THROWS(file.makeGroup("", cls), NeXus::Exception &);
    // check works when correct
    TS_ASSERT_THROWS_NOTHING(file.makeGroup(grp, cls));

    // cleanup
    file.close();
    removeFile(filename);
  }

  void test_open_group() {
    cout << "\ntest openGroup\n";
    string filename("test_nexus_file_grp.h5");
    removeFile(filename);
    NeXus::File file(filename, NXACC_CREATE5);

    // create a group, to be opened
    string grp("test_group"), cls("NXsample");
    file.makeGroup(grp, cls, false);

    // check error conditions
    TS_ASSERT_THROWS(file.openGroup(string(), cls), NeXus::Exception &);
    TS_ASSERT_THROWS(file.openGroup(grp, string()), NeXus::Exception &);

    // now open it, check we are at a different location
    TS_ASSERT_THROWS_NOTHING(file.openGroup(grp, cls));
    auto new_loc = file.getGroupID();
    cout << strmakef("Located at %s\n", new_loc.targetPath);
    TS_ASSERT_DIFFERS(string("/"), string(new_loc.targetPath));

    // cleanup
    file.close();
    removeFile(filename);
  }

  void test_open_group_bad() {
    cout << "\ntest openGroup bad\n";
    string filename("test_nexus_file_grp.h5");
    removeFile(filename);
    NeXus::File file(filename, NXACC_CREATE5);

    // create a group, to be opened
    string grp("test_group"), cls("NXpants");
    file.makeGroup(grp, cls, false);

    // try to open it with wrong class name
    string notcls("NXshorts");
    TS_ASSERT_THROWS(file.openGroup(grp, notcls), NeXus::Exception &);

    // cleanup
    file.close();
    removeFile(filename);
  }

  void test_closeGroup() {
    cout << "\ntest closeGroup\n";
    string filename("test_nexus_file_grp.h5");
    removeFile(filename);
    NeXus::File file(filename, NXACC_CREATE5);

    // check error at root
    TS_ASSERT_THROWS(file.getGroupID(), NeXus::Exception &);

    // now make group, close it, and check we are back at root
    string grp("test_group"), cls("NXsample");
    file.makeGroup(grp, cls, true);
    auto ingrp = file.getGroupID();
    TS_ASSERT_DIFFERS(string("/"), string(ingrp.targetPath));
    file.closeGroup();
    TS_ASSERT_THROWS(file.getGroupID(), NeXus::Exception &)

    // cleanup
    file.close();
    removeFile(filename);
  }

  void test_getPath() {
    cout << "\ntest get_path\n";
    string filename("test_nexus_file_grp.h5");
    removeFile(filename);

    // at root, path should be "/"
    NeXus::File file(filename, NXACC_CREATE5);
    TS_ASSERT_EQUALS("", file.getPath());

    // make and open a group -- now at "/abc"
    file.makeGroup("abc", "NXclass", true);
    TS_ASSERT_EQUALS("/abc", file.getPath());

    // make another layer -- at "/acb/def"
    file.makeGroup("def", "NXentry", true);
    TS_ASSERT_EQUALS("/abc/def", file.getPath());

    // go down a step -- back to "/abc"
    file.closeGroup();
    TS_ASSERT_EQUALS("/abc", file.getPath());

    // go up a different step -- at "/abc/ghi"
    file.makeGroup("ghi", "NXfunsicle", true);
    TS_ASSERT_EQUALS("/abc/ghi", file.getPath());

    // cleanup
    file.close();
    removeFile(filename);
  }
};
