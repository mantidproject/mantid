// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/FileFinder.h"
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
    const std::string filename = Mantid::API::FileFinder::Instance().getFullPath("test_nexus_file_grp.h5");
    File file(filename, NXACC_READ);

    // create a group, to be opened
    string grp("abc"), cls("NXclass");

    // check error conditions
    TS_ASSERT_THROWS(file.openGroup(string(), cls), Exception &);
    TS_ASSERT_THROWS(file.openGroup(grp, string()), Exception &);

    // now open it, check we are at a different location
    TS_ASSERT_THROWS_NOTHING(file.openGroup(grp, cls));
    auto new_loc = file.getGroupID();
    cout << strmakef("Located at %s\n", new_loc.targetPath);
    TS_ASSERT_DIFFERS(string("/"), string(new_loc.targetPath));

    // cleanup
    file.close();
  }

  void test_open_group_bad() {
    cout << "\ntest openGroup bad\n";
    const std::string filename = Mantid::API::FileFinder::Instance().getFullPath("test_nexus_file_grp.h5");
    File file(filename, NXACC_READ);

    string grp("abc"), notgrp("clothes"), cls("NXclass"), notcls("NXpants");

    TS_ASSERT_THROWS(file.openGroup(grp, notcls), Exception &);
    TS_ASSERT_THROWS(file.openGroup(notgrp, cls), Exception &);

    // cleanup
    file.close();
  }

  void test_closeGroup() {
    cout << "\ntest closeGroup\n";
    const std::string filename = Mantid::API::FileFinder::Instance().getFullPath("test_nexus_file_grp.h5");
    File file(filename, NXACC_READ);

    // check error at root
    TS_ASSERT_THROWS(file.getGroupID(), Exception &);

    // open group, close it, and check we are back at root
    file.openGroup("abc", "NXclass");
    auto ingrp = file.getGroupID();
    TS_ASSERT_DIFFERS(string("/"), string(ingrp.targetPath));
    file.closeGroup();
    TS_ASSERT_THROWS(file.getGroupID(), Exception &)

    // cleanup
    file.close();
  }

  void test_getPath() {
    cout << "\ntest get_path\n";
    const std::string filename = Mantid::API::FileFinder::Instance().getFullPath("test_nexus_file_grp.h5");

    // at root, path should be "/"
    File file(filename, NXACC_READ);
    TS_ASSERT_EQUALS("", file.getPath());

    // open a group -- now at "/abc"
    file.openGroup("abc", "NXclass");
    TS_ASSERT_EQUALS("/abc", file.getPath());

    // open another layer -- at "/acb/def"
    file.openGroup("def", "NXentry");
    TS_ASSERT_EQUALS("/abc/def", file.getPath());

    // go down a step -- back to "/abc"
    file.closeGroup();
    TS_ASSERT_EQUALS("/abc", file.getPath());

    // go up a different step -- at "/abc/ghi"
    file.openGroup("ghi", "NXfunsicle");
    TS_ASSERT_EQUALS("/abc/ghi", file.getPath());

    // cleanup
    file.close();
  }
};
