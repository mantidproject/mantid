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
#include "MantidLegacyNexus/NeXusFile.hpp"
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

  void test_open_group_h5() { impl_test_open_group(NexusFormat::HDF5); }
  void test_open_group_h4() { impl_test_open_group(NexusFormat::HDF4); }

  void impl_test_open_group(NexusFormat fmt) {
    cout << "\ntest openGroup\n";
    FormatUniqueVars vars = getFormatUniqueVars(fmt, "test_nexus_file_grp");
    const std::string filename = Mantid::API::FileFinder::Instance().getFullPath(vars.relFilePath).string();
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

  void test_open_group_badh5() { impl_test_open_group_bad(NexusFormat::HDF5); }
  void test_open_group_badh4() { impl_test_open_group_bad(NexusFormat::HDF4); }

  void impl_test_open_group_bad(NexusFormat fmt) {
    cout << "\ntest openGroup bad\n";
    FormatUniqueVars vars = getFormatUniqueVars(fmt, "test_nexus_file_grp");
    const std::string filename = Mantid::API::FileFinder::Instance().getFullPath(vars.relFilePath).string();
    File file(filename, NXACC_READ);

    string grp("abc"), notgrp("clothes"), cls("NXclass"), notcls("NXpants");

    TS_ASSERT_THROWS(file.openGroup(grp, notcls), Exception &);
    TS_ASSERT_THROWS(file.openGroup(notgrp, cls), Exception &);

    // cleanup
    file.close();
  }

  void test_open_group_layers_h5() { impl_test_open_group_layers(NexusFormat::HDF5); }
  void test_open_group_layers_h4() { impl_test_open_group_layers(NexusFormat::HDF4); }

  void impl_test_open_group_layers(NexusFormat fmt) {
    cout << "\ntest openGroup layers\n";
    FormatUniqueVars vars = getFormatUniqueVars(fmt, "test_nexus_file_grp_layers");
    const std::string filename = Mantid::API::FileFinder::Instance().getFullPath(vars.relFilePath).string();
    string grp1("layer1"), grp2("layer2"), cls1("NXpants1"), cls2("NXshorts");

    File file(filename, NXACC_READ);
    // Open group
    file.openGroup(grp1, cls1);

    // Open group player
    file.openGroup(grp2, cls2);
  }

  void test_closeGroup_h5() { impl_test_closeGroup(NexusFormat::HDF5); }
  void test_closeGroup_h4() { impl_test_closeGroup(NexusFormat::HDF4); }

  void impl_test_closeGroup(NexusFormat fmt) {
    cout << "\ntest closeGroup\n";
    FormatUniqueVars vars = getFormatUniqueVars(fmt, "test_nexus_file_grp");
    const std::string filename = Mantid::API::FileFinder::Instance().getFullPath(vars.relFilePath).string();
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

  // #################################################################################################################
  // TEST OPEN / CLOSE DATASET
  // #################################################################################################################

  void test_open_dataset_h5() { impl_test_open_dataset(NexusFormat::HDF5); }
  void test_open_dataset_h4() { impl_test_open_dataset(NexusFormat::HDF4); }

  void impl_test_open_dataset(NexusFormat fmt) {
    cout << "\ntest openData\n";
    FormatUniqueVars vars = getFormatUniqueVars(fmt, "test_nexus_file_data");
    const std::string filename = Mantid::API::FileFinder::Instance().getFullPath(vars.relFilePath).string();

    File file(filename, NXACC_READ);
    file.openGroup("entry", "NXentry");

    string data("test_group");

    // check error conditions
    TS_ASSERT_THROWS(file.openData(string()), Exception &);
    TS_ASSERT_THROWS(file.openData("tacos1"), Exception &);

    // open dataset
    TS_ASSERT_THROWS_NOTHING(file.openData(data));
  }

  void test_closeData_h5() { impl_test_closeData(NexusFormat::HDF5); }
  void test_closeData_h4() { impl_test_closeData(NexusFormat::HDF4); }

  void impl_test_closeData(NexusFormat fmt) {
    cout << "\ntest closeData\n";
    FormatUniqueVars vars = getFormatUniqueVars(fmt, "test_nexus_file_dataclose");
    const std::string filename = Mantid::API::FileFinder::Instance().getFullPath(vars.relFilePath).string();

    File file(filename, NXACC_READ);

    // check error at root
    TS_ASSERT_THROWS(file.closeData(), Exception &);

    // now open data, close it, and check we are back at root
    file.openGroup("entry", "NXentry");
    file.openData("test_data:");
    TS_ASSERT_THROWS_NOTHING(file.closeData());

    TS_ASSERT_THROWS(file.closeData(), Exception &);
  }

  template <typename T> void do_test_data_get(File &file, string name, T in) {
    T out;
    file.openData(name);
    file.getData(&out);
    file.closeData();
    TS_ASSERT_EQUALS(in, out);
  }

  void test_data_get_basic_h5() { impl_test_data_get_basic(NexusFormat::HDF5); }
  void test_data_get_basic_h4() { impl_test_data_get_basic(NexusFormat::HDF4); }

  void impl_test_data_get_basic(NexusFormat fmt) {
    cout << "\ntest dataset read\n";
    FormatUniqueVars vars = getFormatUniqueVars(fmt, "test_nexus_file_dataR_basic");
    // open a file
    const std::string filename = Mantid::API::FileFinder::Instance().getFullPath(vars.relFilePath).string();

    File file(filename, NXACC_READ);
    file.openGroup("entry", "NXentry");

    // get an int
    cout << "\tread int...";
    do_test_data_get<int32_t>(file, "data_int", 12);
    cout << "done\n";

    // get a float
    cout << "\tread float...";
    do_test_data_get<float>(file, "data_float", 1.2f);
    cout << "done\n";

    // get double
    cout << "\tread double...";
    do_test_data_get<double>(file, "data_double", 1.4);
    cout << "done\n";

    // get a single char
    cout << "\tread char...";
    do_test_data_get<char>(file, "data_char", 'x');
    cout << "done\n";

    file.closeGroup();
  }

  void test_data_get_array_h5() { impl_test_data_get_array(NexusFormat::HDF5); }
  void test_data_get_array_h4() { impl_test_data_get_array(NexusFormat::HDF4); }

  void impl_test_data_get_array(NexusFormat fmt) {
    cout << "\ntest dataset read -- arrays\n";
    FormatUniqueVars vars = getFormatUniqueVars(fmt, "test_nexus_file_dataR_array");
    // open a file
    const std::string filename = Mantid::API::FileFinder::Instance().getFullPath(vars.relFilePath).string();

    File file(filename, NXACC_READ);

    // get an int
    file.openGroup("entry", "NXentry");
    file.openData("data_int");
    int in[] = {12, 7, 2, 3}, out[4];
    Info info = file.getInfo();
    file.getData(&(out[0]));
    file.closeData();
    // confirm
    TS_ASSERT_EQUALS(info.dims.size(), 1);
    TS_ASSERT_EQUALS(info.dims.front(), 4);
    for (int i = 0; i < 4; i++) {
      TS_ASSERT_EQUALS(in[i], out[i]);
    }

    // get double array
    file.openData("data_double");
    double ind[] = {12.0, 7.22, 2.3, 3.141592}, outd[4];
    info = file.getInfo();
    file.getData(&(outd[0]));
    file.closeData();
    // confirm
    TS_ASSERT_EQUALS(info.dims.size(), 1);
    TS_ASSERT_EQUALS(info.dims.front(), 4);
    for (int i = 0; i < 4; i++) {
      TS_ASSERT_EQUALS(ind[i], outd[i]);
    }

    // get double 2D array
    std::vector<std::int64_t> dims{3, 2};
    double indd[3][2] = {{12.4, 17.89}, {1256.22, 3.141592}, {0.001, 1.0e4}};
    double outdd[3][2];
    file.openData("data_double_2d");
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
    file.closeGroup();
  }

  void test_data_get_vector_h5() { impl_test_data_get_vector(NexusFormat::HDF5); }
  void test_data_get_vector_h4() { impl_test_data_get_vector(NexusFormat::HDF4); }

  void impl_test_data_get_vector(NexusFormat fmt) {
    cout << "\ntest dataset read -- vector\n";
    FormatUniqueVars vars = getFormatUniqueVars(fmt, "test_nexus_file_dataR_vec");
    // open a file
    const std::string filename = Mantid::API::FileFinder::Instance().getFullPath(vars.relFilePath).string();

    File file(filename, NXACC_READ);
    file.openGroup("entry", "NXentry");

    // get an int vector
    vector<int32_t> in{11, 8, 9, 12}, out;
    file.openData("data_int");
    file.getData(out);
    Info info = file.getInfo();
    file.closeData();
    // confirm
    TS_ASSERT_EQUALS(info.dims.size(), 1);
    TS_ASSERT_EQUALS(info.dims.front(), in.size());
    TS_ASSERT_EQUALS(in, out);

    // get a double vector
    vector<double> ind{101.1, 0.008, 9.1123e12, 12.4}, outd;
    file.openData("data_dbl");
    file.getData(outd);
    info = file.getInfo();
    file.closeData();
    // confirm
    TS_ASSERT_EQUALS(info.dims.size(), 1);
    TS_ASSERT_EQUALS(info.dims.front(), ind.size());
    TS_ASSERT_EQUALS(ind, outd);
  }

  void test_write_access_denied() {
    const std::string filename =
        Mantid::API::FileFinder::Instance().getFullPath("LegacyNexus/hdf4/test_nexus_file_data.h4").string();
    const std::string filename_h5 =
        Mantid::API::FileFinder::Instance().getFullPath("LegacyNexus/hdf5/test_nexus_file_data.h5").string();
    TS_ASSERT_THROWS(File file(filename, NXACC_RDWR), Exception &);
    TS_ASSERT_THROWS(File file(filename, NXACC_CREATE4), Exception &);
    TS_ASSERT_THROWS(File file(filename_h5, NXACC_CREATE5), Exception &);
    TS_ASSERT_THROWS(File file(filename_h5, NXACC_CREATE), Exception &);
    TS_ASSERT_THROWS_NOTHING(File file(filename, NXACC_READ));
  }

  // #################################################################################################################
  // TEST PATH METHODS
  // #################################################################################################################

  void test_getPath_groups_h5() { impl_test_getPath_groups(NexusFormat::HDF5); }
  void test_getPath_groups_h4() { impl_test_getPath_groups(NexusFormat::HDF4); }

  void impl_test_getPath_groups(NexusFormat fmt) {
    cout << "\ntest get_path -- groups only\n";
    FormatUniqueVars vars = getFormatUniqueVars(fmt, "test_nexus_file_grp");
    const std::string filename = Mantid::API::FileFinder::Instance().getFullPath(vars.relFilePath).string();
    File file(filename, NXACC_READ);

    // at root, path should be "/"
    TS_ASSERT_EQUALS("/", file.getPath());

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

  void test_getPath_data_h5() { impl_test_getPath_data(NexusFormat::HDF5); }
  void test_getPath_data_h4() { impl_test_getPath_data(NexusFormat::HDF4); }

  void impl_test_getPath_data(NexusFormat fmt) {
    cout << "\ntest get_path -- groups and data!\n";
    FormatUniqueVars vars = getFormatUniqueVars(fmt, "test_nexus_file_grpdata");
    const std::string filename = Mantid::API::FileFinder::Instance().getFullPath(vars.relFilePath).string();
    File file(filename, NXACC_READ);

    // at root, path should be "/"
    TS_ASSERT_EQUALS("/", file.getPath());

    // open a group -- now at "/abc"
    file.openGroup("abc", "NXentry");
    TS_ASSERT_EQUALS("/abc", file.getPath());

    // make another layer -- at "/acb/def"
    file.openData("def");
    TS_ASSERT_EQUALS("/abc/def", file.getPath());
    file.closeData();
  }

  void test_openPath_h5() { impl_test_openPath(NexusFormat::HDF5); }
  void test_openPath_h4() { impl_test_openPath(NexusFormat::HDF4); }

  void impl_test_openPath(NexusFormat fmt) {
    cout << "\ntest openPath\n";
    FormatUniqueVars vars = getFormatUniqueVars(fmt, "test_nexus_entries");
    fflush(stdout);

    // open a file
    const std::string filename = Mantid::API::FileFinder::Instance().getFullPath(vars.relFilePath).string();
    File file(filename, NXACC_READ);

    // tests invalid cases
    TS_ASSERT_THROWS(file.openPath(""), Exception &);
    // TS_ASSERT_THROWS(file.openPath("entry1"), Exception &);
    TS_ASSERT_THROWS(file.openPath("/pants"), Exception &);
    TS_ASSERT_THROWS(file.openPath("/entry1/pants"), Exception &);

    // make sure we are at root
    file.openPath("/");

    // open the root
    file.openGroup("entry1", "NXentry");
    std::string actual, expected = "/";
    file.openPath("/");
    actual = file.getPath();
    TS_ASSERT_EQUALS(expected, actual);

    expected = "/entry1/layer2b/layer3a";
    file.openPath(expected);
    actual = file.getPath();
    TS_ASSERT_EQUALS(actual, expected);

    expected = "/entry1/layer2a/data1";
    file.openPath(expected);
    actual = file.getPath();
    TS_ASSERT_EQUALS(actual, expected);
  }

  void test_getInfo_h5() { impl_test_getInfo(NexusFormat::HDF5); }
  void test_getInfo_h4() { impl_test_getInfo(NexusFormat::HDF4); }

  void impl_test_getInfo(NexusFormat fmt) {
    cout << "\ntest getInfo -- good\n";
    FormatUniqueVars vars = getFormatUniqueVars(fmt, "test_nexus_file_dataR");
    // open a file
    const std::string filename = Mantid::API::FileFinder::Instance().getFullPath(vars.relFilePath).string();
    File file(filename, NXACC_READ);
    file.openGroup("entry", "NXentry");
    file.openData("int_data");

    // get the info and check
    Info info = file.getInfo();
    TS_ASSERT_EQUALS(info.type, getType<int32_t>());
    TS_ASSERT_EQUALS(info.dims.size(), 1);
    TS_ASSERT_EQUALS(info.dims.front(), 1);

    file.closeData();

    // RE ADD THIS, MAY NEED UNIQUE FILE
    // file.openData("double_data");

    // get the info and check
    // info = file.getInfo();
    // TS_ASSERT_EQUALS(info.type, getType<double>());
    // TS_ASSERT_EQUALS(info.dims.size(), 1);
    // TS_ASSERT_EQUALS(info.dims.front(), 1);
  }

  void test_test_getInfo_bad_h5() { impl_test_getInfo_bad(NexusFormat::HDF5); }
  void test_test_getInfo_bad_h4() { impl_test_getInfo_bad(NexusFormat::HDF4); }

  void impl_test_getInfo_bad(NexusFormat fmt) {
    cout << "\ntest getInfo -- bad\n";
    FormatUniqueVars vars = getFormatUniqueVars(fmt, "test_nexus_file_dataR");
    // open a file
    const std::string filename = Mantid::API::FileFinder::Instance().getFullPath(vars.relFilePath).string();

    File file(filename, NXACC_READ);
    file.openGroup("entry", "NXentry");

    file.openData("int_data");
    ;
    file.closeData();

    // open a group and try to get info
    file.openGroup("a_group", "NXshorts");
    TS_ASSERT_THROWS(file.getInfo(), Exception &);
  }

  // ##################################################################################################################
  // TEST ATTRIBUTE METHODS
  // ################################################################################################################

  template <typename T> void do_test_get_attr(File &file, string name, T const &data) {
    // test get by pointer to data
    T out;
    file.getAttr<T>(name, out);
    TS_ASSERT_EQUALS(data, out);
  }

  void test_get_attr_basic_h5() { impl_test_get_attr_basic(NexusFormat::HDF5); }
  void test_get_attr_basic_h4() { impl_test_get_attr_basic(NexusFormat::HDF4); }

  void impl_test_get_attr_basic(NexusFormat fmt) {
    cout << "\ntest attribute read\n";
    FormatUniqueVars vars = getFormatUniqueVars(fmt, "test_nexus_attr");
    // open a file
    const std::string filename = Mantid::API::FileFinder::Instance().getFullPath(vars.relFilePath).string();
    File file(filename, NXACC_READ);

    // get an int attribute
    do_test_get_attr(file, "int_attr_", 12);

    // get a double attribute
    do_test_get_attr(file, "dbl_attr_", 120.2e6);
  }

  void test_getEntries_h5() { impl_test_getEntries(NexusFormat::HDF5); }
  void test_getEntries_h4() { impl_test_getEntries(NexusFormat::HDF4); }

  void impl_test_getEntries(NexusFormat fmt) {
    cout << "\ntest getEntries\n";
    FormatUniqueVars vars = getFormatUniqueVars(fmt, "test_nexus_entries");
    // open a file
    const std::string filename = Mantid::API::FileFinder::Instance().getFullPath(vars.relFilePath).string();
    File file(filename, NXACC_READ);

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
