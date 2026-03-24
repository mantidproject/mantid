// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/FileFinder.h"
#include "MantidLegacyNexus/NeXusFile.hpp"
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

using namespace Mantid::LegacyNexus;
using namespace LegacyNexusTest;
using std::cout;
using std::endl;
using std::map;
using std::multimap;
using std::string;
using std::vector;

class LegacyNeXusFileReadTest : public CxxTest::TestSuite {

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static LegacyNeXusFileReadTest *createSuite() { return new LegacyNeXusFileReadTest(); }
  static void destroySuite(LegacyNeXusFileReadTest *suite) { delete suite; }

  /**
   * These tests correspond to tests inside napi_test.cpp
   * Refactored to work as unit tests with asserts and comparisons
   * as opposed to a single long print-out test.
   * This test has then been reworked for the LegacyNexus package, to only allow for read operations.
   */

private:
  template <typename T> void do_r_test(File &fileid, std::string const &dataname, T const &data) {
    fileid.openGroup("entry", "NXentry");

    cout << "Testing data " << dataname << "\n";
    // read
    T output;
    fileid.readData(dataname, output);
    fileid.closeGroup();
    // compare
    TS_ASSERT_EQUALS(data, output);
  }

  template <size_t N, size_t M, typename T>
  void do_r2darray_test(File &fileid, std::string const &dataname, T const (&data)[N][M]) {
    fileid.openGroup("entry", "NXentry");

    cout << "Testing attribute " << dataname << "\n";
    // read
    T output[N][M];
    fileid.openData(dataname);
    fileid.getData(&(output[0][0]));
    fileid.closeData();
    fileid.closeGroup();

    // compare
    for (size_t i = 0; i < N; i++) {
      for (size_t j = 0; j < M; j++) {
        TS_ASSERT_EQUALS(data[i][j], output[i][j]);
      }
    }
  }

public:
  void test_napi_char_h5() { impl_test_napi_char(NexusFormat::HDF5); }
  void test_napi_char_h4() { impl_test_napi_char(NexusFormat::HDF4); }

  void impl_test_napi_char(NexusFormat fmt) {
    cout << "Starting NAPI CHAR Test\n";
    FormatUniqueVars vars = getFormatUniqueVars(fmt, "NexusFile_test_char");
    std::string const nxFile(vars.relFilePath);
    const std::string filepath = Mantid::API::FileFinder::Instance().getFullPath(nxFile).string();
    File fileid(filepath);

    // tests of string/char read
    string const ch_test_data = "NeXus ><}&{'\\&\" Data";
    char const c1_array[5][4] = {
        {'a', 'b', 'c', 'd'}, {'e', 'f', 'g', 'h'}, {'i', 'j', 'k', 'l'}, {'m', 'n', 'o', 'p'}, {'q', 'r', 's', 't'}};
    char const c2_array[3][2] = {{'z', 'y'}, {'x', 'w'}, {'v', 'u'}};
    char const c3_array[6][1] = {{'z'}, {'y'}, {'x'}, {'w'}, {'v'}, {'u'}};
    char const c4_array[1][7] = {{'a', 'b', 'c', 'd', 'e', 'f', 'g'}};
    do_r_test(fileid, "ch_data", ch_test_data);
    do_r2darray_test(fileid, "c1_data", c1_array);
    do_r2darray_test(fileid, "c2_data", c2_array);
    do_r2darray_test(fileid, "c3_data", c3_array);
    do_r2darray_test(fileid, "c4_data", c4_array);

    // check all attributes
    fileid.openGroup("entry", "NXentry");
    auto attrs = fileid.getAttrInfos();
    vector<string> exp_attr_names({"hugo", "cucumber"});
    vector<string> attr_names;
    for (auto x : attrs) {
      attr_names.push_back(x.name);
    }
    TS_ASSERT_EQUALS(attr_names, exp_attr_names);

    // check all entries
    vector<string> entry_names({"c1_data", "c2_data", "c3_data", "c4_data", "ch_data"});
    std::map<std::string, std::string> exp_entries;
    for (string x : entry_names) {
      exp_entries[x] = "SDS";
    }
    std::map<std::string, std::string> entries = fileid.getEntries();
    TS_ASSERT_EQUALS(entries, exp_entries);

    // cleanup and return
    fileid.close();
    cout << "napi char test done\n";
  }

  void test_napi_vec_h5() { impl_test_napi_vec(NexusFormat::HDF5); }
  void test_napi_vec_h4() { impl_test_napi_vec(NexusFormat::HDF4); }

  void impl_test_napi_vec(NexusFormat fmt) {
    cout << "Starting NAPI VEC Test\n";
    FormatUniqueVars vars = getFormatUniqueVars(fmt, "NexusFile_test_vec");
    std::string const nxFile(vars.relFilePath);
    const std::string filepath = Mantid::API::FileFinder::Instance().getFullPath(nxFile).string();
    File fileid(filepath);

    // tests of integer read
    vector<unsigned char> const i1_array{1, 2, 3, 4};
    vector<short int> const i2_array{1000, 2000, 3000, 4000};
    vector<int> const i4_array{1000000, 2000000, 3000000, 4000000};
    do_r_test(fileid, "i1_data", i1_array);
    do_r_test(fileid, "i2_data", i2_array);
    do_r_test(fileid, "i4_data", i4_array);

    // tests of float read
    vector<float> const r4_vec{12.f, 13.f, 14.f, 15.f, 16.f};
    vector<double> const r8_vec{12.l, 13.l, 14.l, 15.l, 16.l};
    float const r4_array[5][4] = {
        {1., 2., 3., 4.}, {5., 6., 7., 8.}, {9., 10., 11., 12.}, {13., 14., 15., 16.}, {17., 18., 19., 20.}};
    double const r8_array[5][4] = {
        {1., 2., 3., 4.}, {5., 6., 7., 8.}, {9., 10., 11., 12.}, {13., 14., 15., 16.}, {17., 18., 19., 20.}};
    do_r_test(fileid, "r4_vec_data", r4_vec);
    do_r_test(fileid, "r8_vec_data", r8_vec);
    do_r2darray_test(fileid, "r4_data", r4_array);
    do_r2darray_test(fileid, "r8_data", r8_array);

    // check all entries
    vector<string> entry_names({"i1_data", "i2_data", "i4_data", "r4_data", "r4_vec_data", "r8_data", "r8_vec_data"});
    std::map<std::string, std::string> exp_entries;
    for (string x : entry_names) {
      exp_entries[x] = "SDS";
    }
    fileid.openGroup("entry", "NXentry");
    std::map<std::string, std::string> entries = fileid.getEntries();
    TS_ASSERT_EQUALS(entries, exp_entries);

    // cleanup and return
    fileid.close();
    cout << "napi vec test done\n";
  }

  void test_openPath_h5() { impl_test_openPath(NexusFormat::HDF5); }
  void test_openPath_h4() { impl_test_openPath(NexusFormat::HDF4); }

  void impl_test_openPath(NexusFormat fmt) {
    cout << "tests for openPath\n";
    FormatUniqueVars vars = getFormatUniqueVars(fmt, "NexusFile_openpathtest.nxs");
    string const filename(vars.relFilePath);
    const std::string filepath = Mantid::API::FileFinder::Instance().getFullPath(filename).string();
    File fileid(filepath);

    // compare
    char output;

    fileid.openPath("/entry/data1");
    fileid.getData(&output);
    TS_ASSERT_EQUALS('1', output);

    fileid.openPath("/link/data4");
    fileid.getData(&output);
    TS_ASSERT_EQUALS('4', output);

    fileid.openPath("/entry/data2");
    fileid.getData(&output);
    TS_ASSERT_EQUALS('2', output);

    fileid.openPath("/entry/data/more_data");
    fileid.getData(&output);
    TS_ASSERT_EQUALS('3', output);

    // cleanup
    fileid.close();
    ;
    cout << "NXopenpath checks OK\n";
  }

  void test_links_h5() { impl_test_links(NexusFormat::HDF5); }
  void test_links_h4() { impl_test_links(NexusFormat::HDF4); }

  void impl_test_links(NexusFormat fmt) {
    cout << "tests of linkature\n";
    FormatUniqueVars vars = getFormatUniqueVars(fmt, "NexusFIle_linktest.nxs");
    string const filename(vars.relFilePath);
    const std::string filepath = Mantid::API::FileFinder::Instance().getFullPath(filename).string();
    File fileid(filepath);

    // check data link
    fileid.openPath("/entry/data/some_data");
    // TODO why can't we get the data through the link?
    // string output1;
    // fileid.getData(&output1);
    // TS_ASSERT_EQUALS(somedata, output1);

    NXlink res1 = fileid.getDataID();
    if (fmt == NexusFormat::HDF5) {
      // for HDF4 the `linkType` member is uninitialized.
      TS_ASSERT_EQUALS(1, res1.linkType);
    }
    TS_ASSERT_EQUALS("/entry/some_data", string(res1.targetPath));
    printf("data link works\n");
    fileid.closeGroup();

    // check group link
    fileid.openPath("/entry/group2/group1");
    NXlink res2 = fileid.getGroupID();
    if (fmt == NexusFormat::HDF5) {
      // for HDF4 the `linkType` member is uninitialized.
      TS_ASSERT_EQUALS(0, res2.linkType);
    }

    TS_ASSERT_EQUALS("/entry/group1", string(res2.targetPath));

    // cleanup
    fileid.close();

    printf("group link works\n");
  }
};
