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
#include <H5Cpp.h>
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

  // void setUp() { mkdir( "playground" ); }
  // void tearDown() { system( "rm -Rf playground"); }

  void test_remove() {
    // create a simple file, and make sure removeFile works as intended
    cout << "\nremoving\n";
    string filename("/home/4rx/mantid/build/Testing/Temporary/not_a_real_file.txt");

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

    string filename("/home/4rx/mantid/build/Testing/Temporary/test_nexus_file_init.h5");
    removeFile(filename);

    // create the file and ensure it exists
    NeXus::File file(filename, H5ACC_CREATE5);
    file.close();
    TS_ASSERT(std::filesystem::exists(filename));

    // cleanup
    removeFile(filename);
  }

  void test_can_readwrite() {
    cout << "\ntest readwrite\n";

    // create the file and ensure it exists
    string filename("/home/4rx/mantid/build/Testing/Temporary/test_nexus_file_rdwr.h5");
    NeXus::File file(filename, H5ACC_CREATE5);
    file.close();
    TS_ASSERT(std::filesystem::exists(filename));

    // open in read/write mode, edit it
    file.openFile(filename, H5ACC_RDWR);
    string groupName("test_grp"), input("test"), output;
    file.createGroup(groupName);
    file.setComment(groupName, input);
    output = file.getComment(groupName);
    TS_ASSERT_EQUALS(input, output);
    file.close();

    // cleanup
    removeFile(filename);
  }

  void test_create_readonly() {
    cout << "\ntest readonly\n";

    string grp("test_grp"), expected("teststring"), notexpected("never used"), actual;

    // create the file and ensure it exists
    string filename("/home/4rx/mantid/build/Testing/Temporary/test_nexus_file_read.h5");
    NeXus::File file(filename, H5ACC_CREATE5);
    file.createGroup(grp);
    file.setComment(grp, expected);
    file.close();
    TS_ASSERT(std::filesystem::exists(filename));

    // open in read/write mode, edit it -- should fail, and still be same
    file.openFile(filename, H5ACC_READ);
    TS_ASSERT(file.exists(grp));
    TS_ASSERT_THROWS_ANYTHING(file.setComment(grp, notexpected));
    actual = file.getComment(grp);
    TS_ASSERT_EQUALS(actual, expected);
    TS_ASSERT_DIFFERS(actual, notexpected);
    file.close();

    // cleanup
    removeFile(filename);
  }

  void test_flush() {
    cout << "\ntest flush\n";
    // make sure flush works
    string filename("/home/4rx/mantid/build/Testing/Temporary/test_nexus_file_flush.h5");
    NeXus::File file(filename, H5ACC_CREATE5);
    file.flush();

    // cleanup
    file.close();
    removeFile(filename);
  }

  void test_make_group() {
    cout << "\ntest makeGroup\n";
    string filename("/home/4rx/mantid/build/Testing/Temporary/test_nexus_file_grp.h5");
    removeFile(filename);
    NeXus::File file(filename, H5ACC_CREATE5);
    TS_ASSERT_EQUALS(file.getNumObjs(), 0);

    string grp("test_group"), cls("NXsample");

    // check error conditions
    TS_ASSERT_THROWS(file.makeGroup(grp, ""), NeXus::Exception &);
    TS_ASSERT_THROWS(file.makeGroup("", cls), NeXus::Exception &);
    // check works when correct
    TS_ASSERT_THROWS_NOTHING(file.makeGroup(grp, cls));
    TS_ASSERT_EQUALS(file.getNumObjs(), 1);

    // cleanup
    file.close();
    removeFile(filename);
  }

  void test_open_group() {
    cout << "\ntest openGroup\n";
    string filename("/home/4rx/mantid/build/Testing/Temporary/test_nexus_file_grp.h5");
    removeFile(filename);
    NeXus::File file(filename, H5ACC_CREATE5);

    // get location of root file
    auto loc = file.getCurrentLocation();
    cout << strmakef("Located at %p\n", loc);

    // create a group, to be opened
    string grp("test_group"), cls("NXsample");
    file.makeGroup(grp, cls, false);
    TS_ASSERT_EQUALS(file.getNumObjs(), 1);

    // check error conditions
    TS_ASSERT_THROWS(file.openGroup(string(), cls), NeXus::Exception &);
    TS_ASSERT_THROWS(file.openGroup("tacos1", cls), NeXus::Exception &);
    TS_ASSERT_THROWS(file.openGroup(grp, string()), NeXus::Exception &);

    // now open it, check we are at a different location
    TS_ASSERT_THROWS_NOTHING(file.openGroup(grp, cls));
    auto new_loc = file.getCurrentLocation();
    cout << strmakef("Located at %p\n", new_loc);
    TS_ASSERT_DIFFERS(loc, new_loc);

    // cleanup
    file.close();
    removeFile(filename);
  }

  void test_open_group_bad() {
    cout << "\ntest openGroup bad\n";
    string filename("/home/4rx/mantid/build/Testing/Temporary/test_nexus_file_grp.h5");
    removeFile(filename);
    NeXus::File file(filename, H5ACC_CREATE5);

    // create a group, to be opened
    string grp("test_group"), cls("NXpants");
    file.makeGroup(grp, cls, false);
    TS_ASSERT_EQUALS(file.getNumObjs(), 1);

    // try to open it with wrong class name
    string notcls("NXshorts");
    TS_ASSERT_THROWS(file.openGroup(grp, notcls), NeXus::Exception &);

    // cleanup
    file.close();
    removeFile(filename);
  }

  void test_open_group_layers() {
    cout << "\ntest openGroup layers\n";
    string filename("/home/4rx/mantid/build/Testing/Temporary/test_nexus_file_grp_layers.h5");
    string grp1("layer1"), grp2("layer2"), cls1("NXpants1"), cls2("NXshorts");
    removeFile(filename);

    // create a file with group -- open it
    NeXus::File file(filename, H5ACC_CREATE5);
    file.makeGroup(grp1, cls1, false);
    file.openGroup(grp1, cls1);
    auto layer1 = file.getCurrentLocation();
    TS_ASSERT_EQUALS(file.getNumObjs(), 1);
    TS_ASSERT_EQUALS(layer1->getNumObjs(), 0);

    // create a group inside the group -- open it
    file.makeGroup(grp2, cls2, false);
    file.openGroup(grp2, cls2);
    auto layer2 = file.getCurrentLocation();
    TS_ASSERT_EQUALS(file.getNumObjs(), 1);
    TS_ASSERT_EQUALS(layer1->getNumObjs(), 1);
    TS_ASSERT_DIFFERS(layer1, layer2);

    // cleanup
    file.close();
    removeFile(filename);
  }

  void test_closeGroup() {
    cout << "\ntest closeGroup\n";
    string filename("/home/4rx/mantid/build/Testing/Temporary/test_nexus_file_grp.h5");
    removeFile(filename);
    NeXus::File file(filename, H5ACC_CREATE5);
    auto begin = file.getCurrentLocation();

    // check error at root
    TS_ASSERT_THROWS(file.closeGroup(), NeXus::Exception &);

    // now make group, close it, and check we are back at root
    string grp("test_group"), cls("NXsample");
    file.makeGroup(grp, cls, true);
    auto ingrp = file.getCurrentLocation();
    file.closeGroup();
    auto outgrp = file.getCurrentLocation();

    TS_ASSERT_DIFFERS(outgrp, ingrp);
    TS_ASSERT_EQUALS(outgrp, begin);
    TS_ASSERT_THROWS(file.closeGroup(), NeXus::Exception &);

    // cleanup
    file.close();
    removeFile(filename);
  }

  void test_getPath() {
    cout << "\ntest get_path\n";
    string filename("/home/4rx/mantid/build/Testing/Temporary/test_nexus_file_grp.h5");
    removeFile(filename);

    // at root, path should be "/"
    NeXus::File file(filename, H5ACC_CREATE5);
    TS_ASSERT_EQUALS("/", file.getPath());

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

  void test_makeData() {
    cout << "\ntest make data\n";
    string filename("/home/4rx/mantid/build/Testing/Temporary/test_nexus_file_data.h5");
    removeFile(filename);

    NeXus::File file(filename, H5ACC_CREATE5);
    TS_ASSERT_EQUALS(file.getNumObjs(), 0);

    string name("some_data");
    DimVector dims({1});
    NXnumtype type(NXnumtype::CHAR);

    // check some failing cases
    TS_ASSERT_THROWS(file.makeData("", type, dims), NeXus::Exception &);
    TS_ASSERT_THROWS(file.makeData(name, type, DimVector()), NeXus::Exception &);

    // check it works when it works
    TS_ASSERT_THROWS_NOTHING(file.makeData(name, type, dims, true));
    TS_ASSERT_EQUALS(file.getNumObjs(), 1);

    H5::DataSet data = *(reinterpret_cast<H5::DataSet *>(file.getCurrentLocation()));
    TS_ASSERT_EQUALS(data.getObjName(), "/some_data");

    // cleanup
    file.close();
    removeFile(filename);
  }

  void test_makeData_length() {
    cout << "\ntest make data\n";
    string filename("/home/4rx/mantid/build/Testing/Temporary/test_nexus_file_data.h5");
    removeFile(filename);

    NeXus::File file(filename, H5ACC_CREATE5);
    TS_ASSERT_EQUALS(file.getNumObjs(), 0);

    NXnumtype type (NXnumtype::CHAR);

    // check it works when it works -- int
    string name("some_data_int");
    TS_ASSERT_THROWS_NOTHING(file.makeData(name, type, 3));
    TS_ASSERT_EQUALS(file.getNumObjs(), 1);

    // check it works when it works -- int64_t
    name = "some_data_int64";
    TS_ASSERT_THROWS_NOTHING(file.makeData(name, type, 3L));
    TS_ASSERT_EQUALS(file.getNumObjs(), 2);

    // check it works when it works -- size_t
    name = "some_data_size";
    TS_ASSERT_THROWS_NOTHING(file.makeData(name, type, 3UL));
    TS_ASSERT_EQUALS(file.getNumObjs(), 3);

    // cleanup
    file.close();
    removeFile(filename);
  }

  void test_open_dataset() {
    cout << "\ntest openData\n";
    string filename("/home/4rx/mantid/build/Testing/Temporary/test_nexus_file_data.h5");
    removeFile(filename);
    NeXus::File file(filename, H5ACC_CREATE5);

    // get location of root file
    auto root = file.getCurrentLocation();
    cout << strmakef("Located at %p\n", root);

    // create a dataset, to be opened
    string data("test_group");
    NXnumtype type (NXnumtype::CHAR);
    file.makeData(data, type, 3, false);
    TS_ASSERT_EQUALS(file.getNumObjs(), 1);

    // check error conditions
    TS_ASSERT_THROWS(file.openData(string()), NeXus::Exception &);
    TS_ASSERT_THROWS(file.openData("tacos1"), NeXus::Exception &);

    // now open it, check we are at a different location
    TS_ASSERT_THROWS_NOTHING(file.openData(data));
    auto layer1 = file.getCurrentLocation();
    cout << strmakef("Located at %p\n", layer1);
    TS_ASSERT_DIFFERS(root, layer1);

    // cleanup
    file.close();
    removeFile(filename);
  }

  void test_make_data_layers_bad() {
    cout << "\ntest makeData layers\n";
    string filename("/home/4rx/mantid/build/Testing/Temporary/test_nexus_file_data_layers.h5");
    NXnumtype type (NXnumtype::CHAR);
    string data1("layer1"), data2("layer2");
    removeFile(filename);

    // create a file with data -- open data
    NeXus::File file(filename, H5ACC_CREATE5);
    file.makeData(data1, type, 1, false);
    file.openData(data1);
    auto layer1 = file.getCurrentLocation();
    TS_ASSERT_EQUALS(file.getNumObjs(), 1);

    // try to create a dataset inside the dataset -- this throws an errpr
    TS_ASSERT_THROWS(file.makeData(data2, type, 2, false), NeXus::Exception &);

    // cleanup
    file.close();
    removeFile(filename);
  }

  void test_closeData() {
    cout << "\ntest closeData\n";
    string filename("/home/4rx/mantid/build/Testing/Temporary/test_nexus_file_dataclose.h5");
    removeFile(filename);
    NeXus::File file(filename, H5ACC_CREATE5);
    auto begin = file.getCurrentLocation();

    // check error at root
    TS_ASSERT_THROWS(file.closeGroup(), NeXus::Exception &);

    // now make group, close it, and check we are back at root
    string grp("test_group"), cls("NXsample");
    file.makeGroup(grp, cls, true);
    auto ingrp = file.getCurrentLocation();
    file.closeGroup();
    auto outgrp = file.getCurrentLocation();

    TS_ASSERT_DIFFERS(outgrp, ingrp);
    TS_ASSERT_EQUALS(outgrp, begin);
    TS_ASSERT_THROWS(file.closeGroup(), NeXus::Exception &);

    // cleanup
    file.close();
    removeFile(filename);
  }

};
