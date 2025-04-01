// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidLegacyNexus/NeXusException.hpp"
#include "MantidlegacyNexus/NeXusFile.hpp"
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

using namespace Mantid::LegacyNexus;
using namespace LegacyNexusTest;
using std::cout;
using std::endl;
using std::map;
using std::multimap;
using std::string;
using std::vector;

class LegacyNeXusFileTest : public CxxTest::TestSuite {

public:
  // // This pair of boilerplate methods prevent the suite being created statically
  // // This means the constructor isn't called when running other tests
  static LegacyNeXusFileTest *createSuite() { return new LegacyNeXusFileTest(); }
  static void destroySuite(LegacyNeXusFileTest *suite) { delete suite; }

  void test_open_group() {
    cout << "\ntest openGroup\n";
    string filename("test_nexus_file_grp.h5");
    File file(filename, NXACC_CREATE5);

    // create a group, to be opened
    string grp("test_group"), cls("NXsample");

    // check error conditions
    TS_ASSERT_THROWS(file.openGroup(string(), cls), Exception &);
    TS_ASSERT_THROWS(file.openGroup(grp, string()), Exception &);

    // now open it, check we are at a different location
    TS_ASSERT_THROWS_NOTHING(file.openGroup(grp, cls));
    // auto new_loc = file.getGroupID();
    // cout << strmakef("Located at %s\n", new_loc.targetPath);
    // TS_ASSERT_DIFFERS(string("/"), string(new_loc.targetPath));

    // cleanup
    file.close();
  }

  void test_open_group_bad() {
    cout << "\ntest openGroup bad\n";
    string filename("test_nexus_file_grp.h5");
    File file(filename, NXACC_CREATE5);

    string grp("test_group"), cls("NXpants");

    // try to open it with wrong class name
    string notcls("NXshorts");
    TS_ASSERT_THROWS(file.openGroup(grp, notcls), Exception &);

    // cleanup
    file.close();
  }

  void test_closeGroup() {
    cout << "\ntest closeGroup\n";
    string filename("test_nexus_file_grp.h5");
    File file(filename, NXACC_CREATE5);

    // check error at root
    // TS_ASSERT_THROWS(file.getGroupID(), Exception &);

    // now make group, close it, and check we are back at root
    // string grp("test_group"), cls("NXsample");
    // auto ingrp = file.getGroupID();
    // TS_ASSERT_DIFFERS(string("/"), string(ingrp.targetPath));
    // file.closeGroup();
    // TS_ASSERT_THROWS(file.getGroupID(), Exception &)

    // cleanup
    file.close();
  }

  void test_getPath() {
    cout << "\ntest get_path\n";
    string filename("test_nexus_file_grp.h5");

    // at root, path should be "/"
    File file(filename, NXACC_CREATE5);
    TS_ASSERT_EQUALS("", file.getPath());

    // make and open a group -- now at "/abc"
    TS_ASSERT_EQUALS("/abc", file.getPath());

    // make another layer -- at "/acb/def"
    TS_ASSERT_EQUALS("/abc/def", file.getPath());

    // go down a step -- back to "/abc"
    file.closeGroup();
    TS_ASSERT_EQUALS("/abc", file.getPath());

    // go up a different step -- at "/abc/ghi"
    TS_ASSERT_EQUALS("/abc/ghi", file.getPath());

    // cleanup
    file.close();
  }
};
