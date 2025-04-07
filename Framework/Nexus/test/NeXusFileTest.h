// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidFrameworkTestHelpers/FileResource.h"
#include "MantidLegacyNexus/NeXusException.hpp"
#include "MantidLegacyNexus/NeXusFile.hpp"
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

using namespace Mantid::LegacyNexus;
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

  // #################################################################################################################
  // TEST CONSTRUCTORS
  // #################################################################################################################

  void test_open_group() {
    cout << "\ntest openGroup\n";
    std::string filename = "test_nexus_file_grp_h4.h4";

    File file(filename, NXACC_CREATE4);

    // create a group, to be opened
    string grp("test_group"), cls("NXsample");
    file.makeGroup(grp, cls, false);

    // check error conditions
    TS_ASSERT_THROWS(file.openGroup(string(), cls), Exception &);
    TS_ASSERT_THROWS(file.openGroup("tacos1", cls), Exception &);
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
    std::string filename = "test_nexus_file_grp_h4.h4";

    File file(filename, NXACC_CREATE4);

    // create a group, to be opened
    string grp("test_group"), cls("NXpants");
    file.makeGroup(grp, cls, false);

    // try to open it with wrong class name
    string notcls("NXshorts");
    TS_ASSERT_THROWS(file.openGroup(grp, notcls), Exception &);
    // cleanup
    file.close();
  }

  void test_closeGroup() {
    cout << "\ntest closeGroup\n";
    std::string filename = "test_nexus_file_grp_h4.h4";

    File file(filename, NXACC_CREATE4);

    // check error at root
    TS_ASSERT_THROWS_NOTHING(file.closeGroup());

    // now make group, close it, and check we are back at root
    string grp("test_group"), cls("NXsample");
    file.makeGroup(grp, cls, true);
    file.closeGroup();

    // cleanup
    file.close();
  }

  // #################################################################################################################
  // TEST MAKE / OPEN / PUT / CLOSE DATASET
  // #################################################################################################################

  void test_makeData() {
    cout << "\ntest make data\n";
    std::string filename = "test_nexus_file_data_h4.h4";

    string name("some_data");
    std::vector<std::int64_t> dims({1});
    NXnumtype type(NXnumtype::CHAR);

    File file(filename, NXACC_CREATE4);

    // if there is not a top-level NXentry, should throw error
    TS_ASSERT_THROWS(file.makeData(name, type, dims), Exception &);

    // now make a NXentry group and try
    file.makeGroup("entry", "NXentry", true);

    // check some failing cases
    TS_ASSERT_THROWS(file.makeData("", type, dims), Exception &);
    TS_ASSERT_THROWS(file.makeData(name, type, std::vector<std::int64_t>()), Exception &);

    // check it works when it works
    TS_ASSERT_THROWS_NOTHING(file.makeData(name, type, dims));
  }

  void test_makeData_length() {
    cout << "\ntest make data -- using length\n";
    std::string filename = "test_nexus_file_data_h4.h4";

    File file(filename, NXACC_CREATE4);
    file.makeGroup("entry", "NXentry", true);

    NXnumtype type(NXnumtype::CHAR);

    // check it works when it works -- int
    string name("some_data_int");
    std::int64_t len(3);
    TS_ASSERT_THROWS_NOTHING(file.makeData(name, type, len));
  }

  void test_open_dataset() {
    cout << "\ntest openData\n";
    std::string filename = "test_nexus_file_data_h4.h4";

    File file(filename, NXACC_CREATE4);
    file.makeGroup("entry", "NXentry", true);

    // create a dataset, to be opened
    string data("test_group");
    NXnumtype type(NXnumtype::CHAR);
    file.makeData(data, type, 3, false);

    // check error conditions
    TS_ASSERT_THROWS(file.openData(string()), Exception &);
    TS_ASSERT_THROWS(file.openData("tacos1"), Exception &);

    // now open it, check we are at a different location
    TS_ASSERT_THROWS_NOTHING(file.openData(data));
  }

  void test_closeData() {
    cout << "\ntest closeData\n";
    std::string filename = "test_nexus_file_dataclose_h4.h4";

    File file(filename, NXACC_CREATE4);
    file.makeGroup("entry", "NXentry", true);

    // check error at root
    TS_ASSERT_THROWS(file.closeData(), Exception &);

    // now make data, close it, and check we are back at root
    file.makeData("test_data:", NXnumtype::CHAR, 1, true);
    TS_ASSERT_THROWS_NOTHING(file.closeData());

    TS_ASSERT_THROWS(file.closeData(), Exception &);
  }

  template <typename T> void do_test_data_putget(File &file, string name, T in) {
    T out;
    file.makeData(name, getType<T>(), 1, true);
    file.putData(&in);
    file.getData(&out);
    file.closeData();
    TS_ASSERT_EQUALS(in, out);
  }

  void test_data_putget_basic() {
    cout << "\ntest dataset read/write\n";

    // open a file
    std::string filename = "test_nexus_file_dataRW_h4.h4";

    File file(filename, NXACC_CREATE4);
    file.makeGroup("entry", "NXentry", true);

    // put/get an int
    cout << "\tread/write int...";
    do_test_data_putget<int32_t>(file, "data_int", 12);
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
  }

  void test_putData_bad() {
    cout << "\ntest putData -- bad\n";
    // open a file
    std::string filename = "test_nexus_file_dataRW_h4.h4";

    File file(filename, NXACC_CREATE4);
    file.makeGroup("entry", "NXentry", true);

    // try to put data into a group -- should fail
    int data = 1;
    file.makeGroup("a_group", "NXshirt", true);
    TS_ASSERT_THROWS(file.putData(&data), Exception &);
  }

  void xtest_data_putget_string() {
    cout << "\ntest dataset read/write -- string\n";

    // open a file
    std::string filename = "test_nexus_file_stringrw=_h4.h4";

    File file(filename, NXACC_CREATE4);
    file.makeGroup("entry", "NXentry", true);

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
    std::vector<std::int64_t> dims({(std::int64_t)in.size()});
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
  }

  void test_data_putget_array() {
    cout << "\ntest dataset read/write -- arrays\n";

    // open a file
    std::string filename = "test_nexus_file_dataRW_h4.h4";

    File file(filename, NXACC_CREATE4);
    file.makeGroup("entry", "NXentry", true);

    // put/get an int
    file.makeData("data_int", getType<int32_t>(), 4, true);
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
    file.makeData("data_double", getType<double>(), 4, true);
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
    std::vector<std::int64_t> dims{3, 2};
    double indd[3][2] = {{12.4, 17.89}, {1256.22, 3.141592}, {0.001, 1.0e4}};
    double outdd[3][2];
    file.makeData("data_double_2d", getType<double>(), dims, true);
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
  }

  void test_data_putget_vector() {
    cout << "\ntest dataset read/write -- vector\n";

    // open a file
    std::string filename = "test_nexus_file_dataRW_vec_h4.h4";

    File file(filename, NXACC_CREATE4);
    file.makeGroup("entry", "NXentry", true);

    // put/get an int vector
    vector<int32_t> in{11, 8, 9, 12}, out;
    file.makeData("data_int", getType<int32_t>(), (int64_t)(in.size()), true);
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
    file.makeData("data_dbl", getType<double>(), (int64_t)(ind.size()), true);
    file.putData(ind);
    file.getData(outd);
    info = file.getInfo();
    file.closeData();
    // confirm
    TS_ASSERT_EQUALS(info.dims.size(), 1);
    TS_ASSERT_EQUALS(info.dims.front(), ind.size());
    TS_ASSERT_EQUALS(ind, outd);
  }

  // #################################################################################################################
  // TEST PATH METHODS
  // #################################################################################################################

  void test_getPath_groups() {
    cout << "\ntest get_path -- groups only\n";
    std::string filename = "test_nexus_file_grp_h4.h4";
    File file(filename, NXACC_CREATE4);

    // at root, path should be "/"
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
  }

  void test_getPath_data() {
    cout << "\ntest get_path -- groups and data!\n";
    std::string filename = "test_nexus_file_grpdata_h4.h4";

    File file(filename, NXACC_CREATE4);

    // at root, path should be "/"
    TS_ASSERT_EQUALS("", file.getPath());

    // make and open a group -- now at "/abc"
    file.makeGroup("abc", "NXentry", true);
    TS_ASSERT_EQUALS("/abc", file.getPath());

    // make another layer -- at "/acb/def"
    file.makeData("def", getType<int32_t>(), 1, true);
    int in = 17;
    file.putData(&in);
    TS_ASSERT_EQUALS("/abc/def", file.getPath());
    file.closeData();
  }

  void test_openPath() {
    cout << "\ntest openPath\n";
    fflush(stdout);

    // open a file
    std::string filename = "test_nexus_entries_h4.h4";

    File file(filename, NXACC_CREATE4);

    // setup a recursive group tree
    std::map<std::string, std::string> tree{std::pair<std::string, std::string>{"/entry1", "NXentry"},
                                            std::pair<std::string, std::string>{"/entry1/layer2a", "NXentry"},
                                            std::pair<std::string, std::string>{"/entry1/layer2a/layer3a", "NXentry"},
                                            std::pair<std::string, std::string>{"/entry1/layer2a/layer3b", "NXentry"},
                                            std::pair<std::string, std::string>{"/entry1/layer2a/data1", "SDS"},
                                            std::pair<std::string, std::string>{"/entry1/layer2b", "NXentry"},
                                            std::pair<std::string, std::string>{"/entry1/layer2b/layer3a", "NXentry"},
                                            std::pair<std::string, std::string>{"/entry1/layer2b/layer3b", "NXentry"},
                                            std::pair<std::string, std::string>{"/entry2", "NXentry"},
                                            std::pair<std::string, std::string>{"/entry2/layer2c", "NXentry"},
                                            std::pair<std::string, std::string>{"/entry2/layer2c/layer3c", "NXentry"}};

    string current;
    for (auto it = tree.begin(); it != tree.end(); it++) {
      current = file.getPath();
      string path = it->first;
      while (path.find(current) == path.npos) {
        file.closeGroup();
        current = file.getPath();
      }
      string name = path.substr(path.find_last_of("/") + 1, path.npos);
      if (it->second == "NXentry") {
        file.makeGroup(name, it->second, true);
      } else if (it->second == "SDS") {
        string data = "Data";
        file.makeData(name, NXnumtype::CHAR, (int64_t)(data.size()), true);
        file.putData(data.data());
        file.closeData();
      }
    }
    file.closeGroup();
    file.closeGroup();
    file.closeGroup();

    // tests invalid cases
    TS_ASSERT_THROWS(file.openPath(""), Exception &);
    // TS_ASSERT_THROWS(file.openPath("entry1"), Exception &);
    TS_ASSERT_THROWS(file.openPath("/pants"), Exception &);
    TS_ASSERT_THROWS(file.openPath("/entry1/pants"), Exception &);

    // make sure we are at root
    file.openPath("/");

    // open the root
    file.openGroup("entry1", "NXentry");
    std::string actual, expected = "";
    file.openPath("/");
    actual = file.getPath();
    TS_ASSERT_EQUALS(actual, expected);

    expected = "/entry1/layer2b/layer3a";
    file.openPath(expected);
    actual = file.getPath();
    TS_ASSERT_EQUALS(actual, expected);

    expected = "/entry1/layer2a/data1";
    file.openPath(expected);
    actual = file.getPath();
    TS_ASSERT_EQUALS(actual, expected);
  }

  void test_getInfo() {
    cout << "\ntest getInfo -- good\n";

    // open a file
    std::string filename = "test_nexus_file_dataRW_h4.h4";

    File file(filename, NXACC_CREATE4);
    file.makeGroup("entry", "NXentry", true);

    // put an integer
    int in = 17;
    file.makeData("int_data", getType<int32_t>(), 1, true);
    file.putData(&in);

    // get the info and check
    Info info = file.getInfo();
    TS_ASSERT_EQUALS(info.type, getType<int32_t>());
    TS_ASSERT_EQUALS(info.dims.size(), 1);
    TS_ASSERT_EQUALS(info.dims.front(), 1);

    file.closeData();

    // put a double
    double ind = 107.2345;
    file.makeData("double_data", getType<double>(), 1, true);
    file.putData(&ind);

    // get the info and check
    info = file.getInfo();
    TS_ASSERT_EQUALS(info.type, getType<double>());
    TS_ASSERT_EQUALS(info.dims.size(), 1);
    TS_ASSERT_EQUALS(info.dims.front(), 1);
  }

  void test_getInfo_bad() {
    cout << "\ntest getInfo -- bad\n";
    // open a file
    std::string filename = "test_nexus_file_dataRW_h4.h4";

    File file(filename, NXACC_CREATE4);
    file.makeGroup("entry", "NXentry", true);

    // put an integer
    int in = 17;
    file.makeData("int_data", getType<int32_t>(), 1, true);
    file.putData(&in);
    file.closeData();

    // open a group and try to get info
    file.makeGroup("a_group", "NXshorts", true);
    TS_ASSERT_THROWS(file.getInfo(), Exception &);
  }

  // ##################################################################################################################
  // TEST ATTRIBUTE METHODS
  // ################################################################################################################

  template <typename T> void do_test_putget_attr(File &file, string name, T const &data) {
    // test put/get by pointer to data
    T out;
    file.putAttr<T>(name, data);
    file.getAttr<T>(name, out);
    TS_ASSERT_EQUALS(data, out);
  }

  void test_putget_attr_basic() {
    cout << "\ntest attribute read/write\n";

    // open a file
    std::string filename = "test_nexus_attr_h4.h4";

    File file(filename, NXACC_CREATE4);

    // put/get an int attribute
    do_test_putget_attr(file, "int_attr_", 12);

    // put/get a double attribute
    do_test_putget_attr(file, "dbl_attr_", 120.2e6);
  }

  void test_getEntries() {
    cout << "\ntest getEntries\n";

    // open a file
    std::string filename = "test_nexus_entries_h4.h4";
    File file(filename, NXACC_CREATE4);

    // setup a recursive group tree
    std::map<std::string, std::string> tree{std::pair<std::string, std::string>{"/entry1", "NXentry"},
                                            std::pair<std::string, std::string>{"/entry1/layer2a", "NXentry"},
                                            std::pair<std::string, std::string>{"/entry1/layer2a/layer3a", "NXentry"},
                                            std::pair<std::string, std::string>{"/entry1/layer2a/layer3b", "NXentry"},
                                            std::pair<std::string, std::string>{"/entry1/layer2a/data1", "SDS"},
                                            std::pair<std::string, std::string>{"/entry1/layer2b", "NXentry"},
                                            std::pair<std::string, std::string>{"/entry1/layer2b/layer3a", "NXentry"},
                                            std::pair<std::string, std::string>{"/entry1/layer2b/layer3b", "NXentry"},
                                            std::pair<std::string, std::string>{"/entry2", "NXentry"},
                                            std::pair<std::string, std::string>{"/entry2/layer2c", "NXentry"},
                                            std::pair<std::string, std::string>{"/entry2/layer2c/layer3c", "NXentry"}};

    string current;
    for (auto it = tree.begin(); it != tree.end(); it++) {
      current = file.getPath();
      string path = it->first;
      while (path.find(current) == path.npos) {
        file.closeGroup();
        current = file.getPath();
      }
      string name = path.substr(path.find_last_of("/") + 1, path.npos);
      if (it->second == "NXentry") {
        file.makeGroup(name, it->second, true);
      } else if (it->second == "SDS") {
        string data = "Data";
        file.makeData(name, NXnumtype::CHAR, (int64_t)data.size(), true);
        file.putData(data.data());
        file.closeData();
      }
    }

    file.openPath("/");
    // at root level, should be entry1, entry2
    std::map<std::string, std::string> actual = file.getEntries();
    std::map<std::string, std::string> expected = {std::pair<std::string, std::string>{"entry1", "NXentry"},
                                                   std::pair<std::string, std::string>{"entry2", "NXentry"}};
    for (auto it = expected.begin(); it != expected.end(); it++) {
      TS_ASSERT_EQUALS(actual.count(it->first), 1);
      TS_ASSERT_EQUALS(it->second, actual[it->first]);
    }

    // within entry1, should be layer2a, layer2b
    file.openPath("/entry1");
    actual = file.getEntries();
    expected = std::map<std::string, std::string>({std::pair<std::string, std::string>{"layer2a", "NXentry"},
                                                   std::pair<std::string, std::string>{"layer2b", "NXentry"}});
    for (auto it = expected.begin(); it != expected.end(); it++) {
      TS_ASSERT_EQUALS(actual.count(it->first), 1);
      TS_ASSERT_EQUALS(it->second, actual[it->first]);
    }

    // within entry1/layer2a, should be layer3a, layer3b, data1
    file.openPath("/entry1/layer2a");
    actual = file.getEntries();
    expected = std::map<std::string, std::string>({std::pair<std::string, std::string>{"layer3a", "NXentry"},
                                                   std::pair<std::string, std::string>{"layer3b", "NXentry"},
                                                   std::pair<std::string, std::string>{"data1", "SDS"}});
    for (auto it = expected.begin(); it != expected.end(); it++) {
      TS_ASSERT_EQUALS(actual.count(it->first), 1);
      TS_ASSERT_EQUALS(it->second, actual[it->first]);
    }

    // within entry2/layer2a, should be layer3a, layer3b, data1
    file.openPath("/entry2/layer2c");
    actual = file.getEntries();
    expected = std::map<std::string, std::string>({std::pair<std::string, std::string>{"layer3c", "NXentry"}});
    for (auto it = expected.begin(); it != expected.end(); it++) {
      TS_ASSERT_EQUALS(actual.count(it->first), 1);
      TS_ASSERT_EQUALS(it->second, actual[it->first]);
    }
  }

  // ##################################################################################################################
  // TEST LINK METHODS
  // ################################################################################################################

  /* NOTE these pre-exist, in NeXusFileReadWriteTest.h*/
};
