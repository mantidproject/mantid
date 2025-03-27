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
    cout << "good!\n";
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
    cout << "good!\n";
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
    cout << "good!\n";
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
    cout << "good!\n";
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
    cout << "good!\n";
  }

  void test_getPath_groups() {
    cout << "\ntest get_path -- groups only\n";
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
    cout << "good!\n";
  }

  void test_makeData_length() {
    cout << "\ntest make data -- using length\n";
    string filename("/home/4rx/mantid/build/Testing/Temporary/test_nexus_file_data.h5");
    removeFile(filename);

    NeXus::File file(filename, H5ACC_CREATE5);
    TS_ASSERT_EQUALS(file.getNumObjs(), 0);

    NXnumtype type(NXnumtype::CHAR);

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
    NXnumtype type(NXnumtype::CHAR);
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
    cout << "good!\n";
  }

  void test_make_data_layers_bad() {
    cout << "\ntest makeData layers -- bad\n";
    string filename("/home/4rx/mantid/build/Testing/Temporary/test_nexus_file_data_layers.h5");
    NXnumtype type(NXnumtype::CHAR);
    string data1("layer1"), data2("layer2");
    removeFile(filename);

    // create a file with data -- open data
    NeXus::File file(filename, H5ACC_CREATE5);
    file.makeData(data1, type, 1, false);
    file.openData(data1);
    TS_ASSERT_EQUALS(file.getNumObjs(), 1);

    // try to create a dataset inside the dataset -- this throws an errpr
    TS_ASSERT_THROWS(file.makeData(data2, type, 2, false), NeXus::Exception &);

    // cleanup
    file.close();
    removeFile(filename);
    cout << "good!\n";
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
    cout << "good!\n";
  }

  template <typename T> void do_test_data_putget(NeXus::File &file, string name, T in) {
    T out;
    file.makeData(name, NeXus::getType<T>(), 1, true);
    file.putData(&in);
    file.getData(&out);
    file.closeData();
    TS_ASSERT_EQUALS(in, out);
  }

  void test_data_putget_basic() {
    cout << "\ntest dataset read/write\n";

    // open a file
    string filename("/home/4rx/mantid/build/Testing/Temporary/test_nexus_file_dataRW.h5");
    removeFile(filename);
    NeXus::File file(filename, H5ACC_CREATE5);

    // put/get an int
    cout << "\tread/write int...";
    do_test_data_putget<int>(file, "data_int", 12);
    cout << "done\n";

    // put/get an int64_t
    cout << "\tread/write int64_t...";
    do_test_data_putget<int64_t>(file, "data_int64", 12ll);
    cout << "done\n";

    // put/get a size_t
    cout << "\tread/write size_T...";
    do_test_data_putget<size_t>(file, "data_sizet", 12ull);
    cout << "done\n";

    // put/get a float
    cout << "\tread/write float...";
    do_test_data_putget<float>(file, "data_float", 1.2f);
    cout << "done\n";

    // put/get double
    cout << "\tread/write double...";
    do_test_data_putget<double>(file, "data_double", 1.4);
    cout << "done\n";

    // put/get a single char
    cout << "\tread/write char...";
    do_test_data_putget<char>(file, "data_char", 'x');
    cout << "done\n";

    // cleanup
    file.close();
    removeFile(filename);
  }

  void test_putData_bad() {
    cout << "\ntest putData -- bad\n";
    // open a file
    string filename("/home/4rx/mantid/build/Testing/Temporary/test_nexus_file_dataRW.h5");
    removeFile(filename);
    NeXus::File file(filename, H5ACC_CREATE5);

    int data = 1;
    file.makeGroup("a_group", "NXshirt", true);
    TS_ASSERT_THROWS(file.putData<int>(&data), NeXus::Exception &);

    // cleanup
    file.close();
    removeFile(filename);
    cout << "good!";
  }

  void test_getPath_data() {
    cout << "\ntest get_path -- groups and data!\n";
    string filename("/home/4rx/mantid/build/Testing/Temporary/test_nexus_file_grpdata.h5");
    removeFile(filename);

    // at root, path should be "/"
    NeXus::File file(filename, H5ACC_CREATE5);
    TS_ASSERT_EQUALS("/", file.getPath());

    // make and open a group -- now at "/abc"
    file.makeGroup("abc", "NXclass", true);
    TS_ASSERT_EQUALS("/abc", file.getPath());

    // make another layer -- at "/acb/def"
    file.makeData("def", NeXus::getType<int>(), 1, true);
    int in=17;
    file.putData(&in);
    TS_ASSERT_EQUALS("/abc/def", file.getPath());
    file.closeData();

    // cleanup
    file.close();
    removeFile(filename);
  }

  void test_getInfo() {
    cout << "\ntest getInfo -- good\n";

    // open a file
    string filename("/home/4rx/mantid/build/Testing/Temporary/test_nexus_file_dataRW.h5");
    removeFile(filename);
    NeXus::File file(filename, H5ACC_CREATE5);

    // put an integer
    int in = 17;
    file.makeData("int_data", NeXus::getType<int>(), 1, true);
    file.putData(&in);

    // get the info and check
    Info info = file.getInfo();
    TS_ASSERT_EQUALS(info.type, NeXus::getType<int>());
    TS_ASSERT_EQUALS(info.dims.size(), 1);
    TS_ASSERT_EQUALS(info.dims.front(), 1);

    file.closeData();

    // put a double
    double ind = 107.2345;
    file.makeData("double_data", NeXus::getType<double>(), 1, true);
    file.putData(&ind);

    // get the info and check
    info = file.getInfo();
    TS_ASSERT_EQUALS(info.type, NeXus::getType<double>());
    TS_ASSERT_EQUALS(info.dims.size(), 1);
    TS_ASSERT_EQUALS(info.dims.front(), 1);

    // cleanup
    file.close();
    removeFile(filename);
    cout << "good!";
  }

  void test_getInfo_bad() {
    cout << "\ntest getInfo -- bad\n";
    // open a file
    string filename("/home/4rx/mantid/build/Testing/Temporary/test_nexus_file_dataRW.h5");
    removeFile(filename);
    NeXus::File file(filename, H5ACC_CREATE5);

    // put an integer
    int in = 17;
    file.makeData("int_data", NeXus::getType<int>(), 1, true);
    file.putData(&in);
    file.closeData();

    // open a group and try to get info
    file.makeGroup("a_group", "NXshorts", true);
    TS_ASSERT_THROWS(file.getInfo(), NeXus::Exception &);

    // cleanup
    file.close();
    removeFile(filename);
    cout << "good!\n";
  }

  void test_data_putget_string() {
    cout << "\ntest dataset read/write -- string\n";

    // open a file
    string filename("/home/4rx/mantid/build/Testing/Temporary/test_nexus_file_stringrw=.h5");
    removeFile(filename);
    NeXus::File file(filename, H5ACC_CREATE5);

    // put/get a string
    cout << "\nread/write string...\n";
    string in("this is a string"), out;
    file.makeData("string_data", NXnumtype::CHAR, in.size(), true);
    file.putData(&in);
    file.getData(&out);
    file.closeData();
    TS_ASSERT_EQUALS(in, out);

    // do it another way
    in = "this is some different data";
    DimVector dims({(dimsize_t)in.size()});
    file.makeData("more_string_data", NXnumtype::CHAR, dims, true);
    file.putData(&in);
    file.getData(&out);
    file.closeData();
    TS_ASSERT_EQUALS(in, out);

    // yet another way
    in = "even more data";
    file.makeData("string_data_2", NXnumtype::CHAR, in.size(), true);
    file.putData(&in);
    out = file.getStrData();
    TS_ASSERT_EQUALS(in, out);

    // cleanup
    file.close();
    removeFile(filename);
    cout << "good!\n";
  }

  void test_data_putget_array() {
    cout << "\ntest dataset read/write -- arrays\n";

    // open a file
    string filename("/home/4rx/mantid/build/Testing/Temporary/test_nexus_file_dataRW.h5");
    removeFile(filename);
    NeXus::File file(filename, H5ACC_CREATE5);

    // put/get an int
    file.makeData("data_int", NeXus::getType<int>(), 4, true);
    int in[] = {12, 7, 2, 3}, out[4];
    file.putData(&(in[0]));
    Info info = file.getInfo();
    file.getData(&(out[0]));
    file.closeData();
    // confirm
    TS_ASSERT_EQUALS(info.dims.size(), 1);
    TS_ASSERT_EQUALS(info.dims.front(), 4);
    for (int i = 0; i < 4; i++) {
      TS_ASSERT_EQUALS(in[i], out[i]);
    }

    // put/get double array
    file.makeData("data_double", NeXus::getType<double>(), 4, true);
    double ind[] = {12.0, 7.22, 2.3, 3.141592}, outd[4];
    file.putData(&(ind[0]));
    info = file.getInfo();
    file.getData(&(outd[0]));
    file.closeData();
    // confirm
    TS_ASSERT_EQUALS(info.dims.size(), 1);
    TS_ASSERT_EQUALS(info.dims.front(), 4);
    for (int i = 0; i < 4; i++) {
      TS_ASSERT_EQUALS(ind[i], outd[i]);
    }

    // put/get double 2D array
    DimVector dims{3, 2};
    double indd[3][2] = {{12.4, 17.89}, {1256.22, 3.141592}, {0.001, 1.0e4}};
    double outdd[3][2];
    file.makeData("data_double_2d", NeXus::getType<double>(), dims, true);
    file.putData(&(indd[0][0]));
    info = file.getInfo();
    file.getData(&(outdd[0][0]));
    file.closeData();
    // confirm
    TS_ASSERT_EQUALS(info.dims.size(), 2);
    TS_ASSERT_EQUALS(info.dims.front(), 3);
    TS_ASSERT_EQUALS(info.dims.back(), 2);
    for (int i = 0; i < dims[0]; i++) {
      for (int j = 0; j < dims[1]; j++) {
        TS_ASSERT_EQUALS(indd[i][j], outdd[i][j]);
      }
    }

    // put/get a char array
    char word[] = "silicovolcaniosis";
    char read[18];
    file.makeData("data_char", NeXus::getType<char>(), 18, true);
    file.putData(word);
    info = file.getInfo();
    file.getData(read);
    file.closeData();
    // confirm
    TS_ASSERT_EQUALS(info.dims.size(), 1);
    TS_ASSERT_EQUALS(info.dims.front(), 18);
    TS_ASSERT_EQUALS(string(word), string(read));

    // cleanup
    file.close();
    removeFile(filename);
  }

  void test_data_putget_vector() {
    cout << "\ntest dataset read/write -- vector\n";

    // open a file
    string filename("/home/4rx/mantid/build/Testing/Temporary/test_nexus_file_dataRW_vec.h5");
    removeFile(filename);
    NeXus::File file(filename, H5ACC_CREATE5);

    // put/get an int vector
    vector<int> in{11, 8, 9, 12}, out;
    file.makeData("data_int", NeXus::getType<int>(), in.size(), true);
    file.putData(in);
    file.getData(out);
    Info info = file.getInfo();
    file.closeData();
    // confirm
    TS_ASSERT_EQUALS(info.dims.size(), 1);
    TS_ASSERT_EQUALS(info.dims.front(), in.size());
    TS_ASSERT_EQUALS(in, out);

    // put/get a double vector
    vector<double> ind{101.1, 0.008, 9.1123e12, 12.4}, outd;
    file.makeData("data_dbl", NeXus::getType<double>(), ind.size(), true);
    file.putData(ind);
    file.getData(outd);
    info = file.getInfo();
    file.closeData();
    // confirm
    TS_ASSERT_EQUALS(info.dims.size(), 1);
    TS_ASSERT_EQUALS(info.dims.front(), ind.size());
    TS_ASSERT_EQUALS(ind, outd);

    // cleanup
    file.close();
    removeFile(filename);
  }

  template <typename T> void do_test_putget_attr(NeXus::File &file, string name, T const &data) {
    // test put/get by pointer to data
    T out;
    file.putAttr<T>(name, data);
    file.getAttr<T>(name, out);
    TS_ASSERT_EQUALS(data, out);
  }

  void test_putget_attr_basic() {
    cout << "\ntest attribute read/write\n";

    // open a file
    string filename("/home/4rx/mantid/build/Testing/Temporary/test_nexus_attr.h5");
    removeFile(filename);
    NeXus::File file(filename, H5ACC_CREATE5);

    // put/get an int attribute
    do_test_putget_attr(file, "int_attr_", 12);

    // put/get a double attribute
    do_test_putget_attr(file, "dbl_attr_", 120.2e6);

    // put/get a single char attribute
    do_test_putget_attr(file, "char_attr_", 'x');

    // put/get a string attribute
    string data5 = "different string of text";
    do_test_putget_attr(file, "str_attr_", data5);

    // cleanup
    file.close();
    removeFile(filename);
    cout << "good!\n";
  }

  void test_getEntries() {
    cout << "\ntest getEntries\n";

    // open a file
    string filename("/home/4rx/mantid/build/Testing/Temporary/test_nexus_entries.h5");
    removeFile(filename);
    NeXus::File file(filename, H5ACC_CREATE5);

    // setup a recursive group tree
    Entries expected {
      Entry {"/entry1", "NXentry"},
      Entry {"/entry1/layer2a", "NXentry"},
      Entry {"/entry1/layer2a/layer3a", "NXentry"},
      Entry {"/entry1/layer2a/layer3b", "NXentry"},
      Entry {"/entry1/layer2a/data1", "SDS"},
      Entry {"/entry1/layer2b", "NXentry"},
      Entry {"/entry1/layer2b/layer3a", "NXentry"},
      Entry {"/entry1/layer2b/layer3b", "NXentry"},
      Entry {"/entry2", "NXentry"},
      Entry {"/entry2/layer2c", "NXentry"},
      Entry {"/entry2/layer2c/layer3c", "NXentry"}
    };

    string current;
    for (auto it = expected.begin(); it != expected.end(); it++) {
      current = file.getPath();
      string path = it->first;
      while (path.find(current) == path.npos) {
        file.closeGroup();
        current = file.getPath();
      }
      string name = path.substr(path.find_last_of("/")+1, path.npos);
      if (it->second == "NXentry") {
        file.makeGroup(name, it->second, true);
      } else if (it->second == "SDS") {
        string data = "Data";
        file.makeData(name, NXnumtype::CHAR, data.size(), true);
        file.putData(data.data());
        file.closeData();
      }
    } 

    Entries actual = file.getEntries();
    for(auto it = expected.begin(); it != expected.end(); it++) {
      TS_ASSERT_EQUALS(actual.count(it->first), 1);
      TS_ASSERT_EQUALS(it->second, actual[it->first]);
    }

    // also test root level name
    TS_ASSERT_EQUALS("entry1", file.getTopLevelEntryName());
  }

  void test_links() {
    cout << "tests of linkature\n";

    string const filename("NexusFIle_linktest.nxs");
    removeFile(filename);

    NeXus::File file(filename, H5ACC_CREATE5);
    file.makeGroup("entry", "NXentry", true);

    // Create some data with a link
    cout << "create entry at /entry/some_data\n";
    string const somedata("this is some data");
    file.makeData("some_data", NXnumtype::CHAR, DimVector({(dimsize_t)somedata.size()}));
    file.openData("some_data");
    file.putData(somedata.data());
    NXlink datalink = file.getDataID();
    file.closeData();
    file.flush();
    // Create a group, and link it to that data
    cout << "create group at /entry/data to link to the data\n";
    file.makeGroup("data", "NXdata");
    file.openGroup("data", "NXdata");
    file.makeLink(datalink);
    file.closeGroup();
    file.flush();

    // // check data link
    // file.openPath("/entry/data/some_data");
    // // TODO why can't we get the data through the link?
    // // string output1;
    // // fileid.getData(&output1);
    // // TS_ASSERT_EQUALS(somedata, output1);
    // NXlink res1 = file.getDataID();
    // TS_ASSERT_EQUALS(datalink.linkType, res1.linkType);
    // TS_ASSERT_EQUALS(string(datalink.targetPath), string(res1.targetPath));
    // printf("data link works\n");
    // fileid.closeGroup();

    // // Create two groups, group1 and group2
    // // Make a link inside group2 to group1
    // // make group1
    // cout << "create group /entry/group1\n";
    // std::string const strdata("NeXus sample data");
    // fileid.makeGroup("group1", "NXentry");
    // fileid.openGroup("group1", "NXentry");
    // NXlink grouplink = fileid.getGroupID();
    // fileid.closeGroup();
    // // make group 2
    // cout << "create group /entry/group2/group1\n";
    // fileid.makeGroup("group2", "NXentry");
    // fileid.openGroup("group2", "NXentry");
    // fileid.makeLink(grouplink);
    // fileid.closeGroup();

    // // check group link
    // fileid.openPath("/entry/group2/group1");
    // NXlink res2 = file.getGroupID();
    // TS_ASSERT_EQUALS(grouplink.linkType, res2.linkType);
    // TS_ASSERT_EQUALS(string(grouplink.targetPath), string(res2.targetPath));
    // printf("group link works\n");
  }
};
