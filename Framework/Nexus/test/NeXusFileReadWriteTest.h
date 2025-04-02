// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidNexus/NeXusFile.hpp"
#include "test_helper.h"
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
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

class NeXusFileReadWriteTest : public CxxTest::TestSuite {

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static NeXusFileReadWriteTest *createSuite() { return new NeXusFileReadWriteTest(); }
  static void destroySuite(NeXusFileReadWriteTest *suite) { delete suite; }

  /**
   * These tests correspond to tests inside napi_test.cpp
   * Refactored to work as unit tests with asserts and comparisons
   * as opposed to a single long print-out test
   */

private:
  File do_prep_files(std::string const nxFile) {
    removeFile(nxFile); // in case previous run didn't clean up

    cout << "Creating \"" << nxFile << "\"" << endl;
    // create file
    File fileid(nxFile, NXACC_CREATE5);

    fileid.makeGroup("entry", "NXentry");
    fileid.openGroup("entry", "NXentry");
    fileid.putAttr("hugo", "namenlos");
    fileid.putAttr("cucumber", "passion");
    return fileid;
  }

  template <typename T> void do_rw_test(File &fileid, std::string const &dataname, T const &data) {
    cout << "Testing data " << dataname << "\n";
    // write
    fileid.writeData(dataname, data);

    // read
    T output;
    fileid.readData(dataname, output);

    // compare
    TS_ASSERT_EQUALS(data, output);
  }

  template <size_t N, size_t M, typename T>
  void do_rw2darray_test(File &fileid, std::string const &dataname, T const (&data)[N][M]) {
    cout << "Testing attribute " << dataname << "\n";
    // write
    fileid.makeData(dataname, getType<T>(), DimVector({N, M}));
    fileid.openData(dataname);
    fileid.putData(&(data[0][0]));
    fileid.closeData();

    // read
    T output[N][M];
    fileid.openData(dataname);
    fileid.getData(&(output[0][0]));
    fileid.closeData();

    // compare
    for (size_t i = 0; i < N; i++) {
      for (size_t j = 0; j < M; j++) {
        TS_ASSERT_EQUALS(data[i][j], output[i][j]);
      }
    }
  }

  template <typename T> void do_rwslabvec_test(File &fileid, std::string const dataname, vector<T> const &data) {
    cout << "Testing slab " << dataname << "\n";

    // write
    dimsize_t dimsize = data.size();
    DimSizeVector const start({0}), size({dimsize});
    fileid.makeData(dataname, getType<T>(), dimsize);
    fileid.openData(dataname);
    fileid.putSlab(data, start, size);
    fileid.closeData();

    // read
    int const Ncheck(5); // can't use variable-length arrays, just check this many
    T output[Ncheck];
    fileid.openData(dataname);
    fileid.getSlab(&(output[0]), start, size);

    // compare
    for (int i = 0; i < Ncheck; i++) {
      TS_ASSERT_EQUALS(data[i], output[i])
    }
  }

  template <size_t N, size_t M, typename T>
  void do_rwslab_test(File &fileid, std::string const dataname, T const (&data)[N][M]) {
    cout << "Testing slab " << dataname << "\n";

    // write
    DimSizeVector start({0, 0}), size({N, M});
    DimVector const dims({N, M});
    fileid.makeData(dataname, getType<T>(), dims);
    fileid.openData(dataname);
    fileid.putSlab(&(data[0][0]), start, size);
    fileid.closeData();

    // prepare to read/compare
    T output[N][M];
    fileid.openData(dataname);

    // read, compare, row-by-row
    for (size_t i = 1; i <= N; i++) {
      size = {(dimsize_t)i, (dimsize_t)M};
      fileid.getSlab(&(output[0][0]), start, size);
      for (size_t j = 0; j < M; j++) {
        TS_ASSERT_EQUALS(data[0][j], output[0][j]);
      }
    }
  }

public:
  void test_napi_char() {
    cout << "Starting NAPI CHAR Test\n";
    std::string const nxFile("NexusFile_test_char.h5");
    File fileid = do_prep_files(nxFile);

    // tests of string/char read/write
    string const ch_test_data = "NeXus ><}&{'\\&\" Data";
    char const c1_array[5][4] = {
        {'a', 'b', 'c', 'd'}, {'e', 'f', 'g', 'h'}, {'i', 'j', 'k', 'l'}, {'m', 'n', 'o', 'p'}, {'q', 'r', 's', 't'}};
    char const c2_array[3][2] = {{'z', 'y'}, {'x', 'w'}, {'v', 'u'}};
    char const c3_array[6][1] = {{'z'}, {'y'}, {'x'}, {'w'}, {'v'}, {'u'}};
    char const c4_array[1][7] = {{'a', 'b', 'c', 'd', 'e', 'f', 'g'}};
    do_rw_test(fileid, "ch_data", ch_test_data);
    do_rw2darray_test(fileid, "c1_data", c1_array);
    do_rw2darray_test(fileid, "c2_data", c2_array);
    do_rw2darray_test(fileid, "c3_data", c3_array);
    do_rw2darray_test(fileid, "c4_data", c4_array);

    // check all attributes
    auto attrs = fileid.getAttrInfos();
    vector<string> exp_attr_names({"hugo", "cucumber"});
    vector<string> attr_names;
    for (auto x : attrs) {
      attr_names.push_back(x.name);
    }
    TS_ASSERT_EQUALS(attr_names, exp_attr_names);

    // check all entries
    vector<string> entry_names({"c1_data", "c2_data", "c3_data", "c4_data", "ch_data"});
    Entries exp_entries;
    for (string x : entry_names) {
      exp_entries[x] = "SDS";
    }
    Entries entries = fileid.getEntries();
    TS_ASSERT_EQUALS(entries, exp_entries);

    // cleanup and return
    fileid.close();
    cout << "napi slab test done\n";
    removeFile(nxFile);
  }

  void test_napi_vec() {
    cout << "Starting NAPI VEC Test\n";
    std::string const nxFile("NexusFile_test_vec.h5");
    File fileid = do_prep_files(nxFile);

    // tests of integer read/write
    vector<uint8_t> const i1_array{1, 2, 3, 4};
    vector<int16_t> const i2_array{1000, 2000, 3000, 4000};
    vector<int32_t> const i4_array{1000000, 2000000, 3000000, 4000000};
    do_rw_test(fileid, "i1_data", i1_array);
    do_rw_test(fileid, "i2_data", i2_array);
    do_rw_test(fileid, "i4_data", i4_array);

    // tests of float read/write
    vector<float> const r4_vec{12.f, 13.f, 14.f, 15.f, 16.f};
    vector<double> const r8_vec{12., 13., 14., 15., 16.};
    float const r4_array[5][4] = {
        {1., 2., 3., 4.}, {5., 6., 7., 8.}, {9., 10., 11., 12.}, {13., 14., 15., 16.}, {17., 18., 19., 20.}};
    double const r8_array[5][4] = {
        {1., 2., 3., 4.}, {5., 6., 7., 8.}, {9., 10., 11., 12.}, {13., 14., 15., 16.}, {17., 18., 19., 20.}};
    do_rw_test(fileid, "r4_vec_data", r4_vec);
    do_rw_test(fileid, "r8_vec_data", r8_vec);
    do_rw2darray_test(fileid, "r4_data", r4_array);
    do_rw2darray_test(fileid, "r8_data", r8_array);

    // check all entries
    vector<string> entry_names({"i1_data", "i2_data", "i4_data", "r4_data", "r4_vec_data", "r8_data", "r8_vec_data"});
    Entries exp_entries;
    for (string x : entry_names) {
      exp_entries[x] = "SDS";
    }
    Entries entries = fileid.getEntries();
    TS_ASSERT_EQUALS(entries, exp_entries);

    // cleanup and return
    fileid.close();
    cout << "napi slab test done\n";
    removeFile(nxFile);
  }

  void test_napi_slab() {
    cout << "Starting NAPI SLAB Test\n";
    std::string const nxFile("NExusFile_test_vec.h5");
    File fileid = do_prep_files(nxFile);

    // test of slab read/write
    vector<float> const r4_vec{12.f, 13.f, 14.f, 15.f, 16.f};
    vector<double> const r8_vec{12., 13., 14., 15., 16.};
    float const r4_array[5][4] = {
        {1., 2., 3., 4.}, {5., 6., 7., 8.}, {9., 10., 11., 12.}, {13., 14., 15., 16.}, {17., 18., 19., 20.}};
    double const r8_array[5][4] = {
        {1., 2., 3., 4.}, {5., 6., 7., 8.}, {9., 10., 11., 12.}, {13., 14., 15., 16.}, {17., 18., 19., 20.}};
    do_rwslabvec_test(fileid, "r4_slab", r4_vec);
    do_rwslabvec_test(fileid, "r8_slab", r8_vec);
    do_rwslab_test(fileid, "r4_slab2d", r4_array);
    do_rwslab_test(fileid, "r8_slab2d", r8_array);

    // check all entries
    vector<string> entry_names({"r4_slab", "r4_slab2d", "r8_slab", "r8_slab2d"});
    Entries exp_entries;
    for (string x : entry_names) {
      exp_entries[x] = "SDS";
    }
    Entries entries = fileid.getEntries();
    TS_ASSERT_EQUALS(entries, exp_entries);

    // cleanup and return
    fileid.close();
    cout << "napi slab test done\n";
    removeFile(nxFile);
  }

  void test_openPath() {
    cout << "tests for openPath\n";

    // make file with path /entry
    string const filename("NexusFile_openpathtest.nxs");
    File fileid = do_prep_files(filename);

    // make path /entry/data1
    fileid.writeData("data1", '1');

    // make path /entry/data2
    fileid.writeData("data2", '2');

    // make path /entry/data1/more_data
    fileid.makeGroup("data", "NXdata");
    fileid.openGroup("data", "NXdata");
    fileid.writeData("more_data", '3');

    // make path /link
    fileid.closeGroup(); // close /entry/data
    fileid.closeGroup(); // close /entry
    fileid.makeGroup("link", "NXentry");
    fileid.openGroup("link", "NXentry"); // open /link
    fileid.writeData("data4", '4');

    // compare
    char output;
    fileid.closeGroup();

    fileid.openPath("/entry/data1");
    fileid.getData(&output);
    TS_ASSERT_EQUALS('1', output);

    fileid.openPath("/link/data4");
    fileid.getData(&output);
    TS_ASSERT_EQUALS('4', output);

    fileid.openPath("/entry/data/more_data");
    fileid.getData(&output);
    TS_ASSERT_EQUALS('3', output);

    fileid.openData("/entry/data2");
    fileid.getData(&output);
    TS_ASSERT_EQUALS('2', output);

    // cleanup
    fileid.close();
    removeFile(filename);
    cout << "NXopenpath checks OK\n";
  }

  void test_links() {
    cout << "tests of linkature\n";

    string const filename("NexusFIle_linktest.nxs");
    removeFile(filename);
    File fileid = do_prep_files(filename);

    // Create some data with a link
    cout << "create entry at /entry/some_data\n";
    string const somedata("this is some data");
    fileid.makeData("some_data", NXnumtype::CHAR, DimVector({(dimsize_t)somedata.size()}));
    fileid.openData("some_data");
    fileid.putData(somedata.c_str());
    NXlink datalink = fileid.getDataID();
    fileid.closeData();
    fileid.flush();

    // Create a group, and link it to that data
    cout << "create group at /entry/data to link to the data\n";
    fileid.makeGroup("data", "NXdata");
    fileid.openGroup("data", "NXdata");
    fileid.makeLink(datalink);
    fileid.closeGroup();
    fileid.flush();

    // check data link
    fileid.openPath("/entry/data/some_data");
    // TODO why can't we get the data through the link?
    // string output1;
    // fileid.getData(&output1);
    // TS_ASSERT_EQUALS(somedata, output1);
    NXlink res1 = fileid.getDataID();
    TS_ASSERT_EQUALS(datalink.linkType, res1.linkType);
    TS_ASSERT_EQUALS(string(datalink.targetPath), string(res1.targetPath));
    cout << "data link works\n";

    fileid.openPath("/entry");

    // Create two groups, group1 and group2
    // Make a link inside group2 to group1
    // make group1
    cout << "create group /entry/group1\n";
    std::string const strdata("NeXus sample data");
    fileid.makeGroup("group1", "NXentry");
    fileid.openGroup("group1", "NXentry");
    NXlink grouplink = fileid.getGroupID();
    fileid.closeGroup();

    // make group 2
    cout << "create group /entry/group2/group1\n";
    fileid.makeGroup("group2", "NXentry");
    fileid.openGroup("group2", "NXentry");
    fileid.makeLink(grouplink);
    fileid.closeGroup();

    // check group link
    fileid.openPath("/entry/group2/group1");
    NXlink res2 = fileid.getGroupID();
    TS_ASSERT_EQUALS(grouplink.linkType, res2.linkType);
    TS_ASSERT_EQUALS(string(grouplink.targetPath), string(res2.targetPath));
    cout << "group link works\n";
  }
};
