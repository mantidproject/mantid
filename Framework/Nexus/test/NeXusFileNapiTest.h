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

namespace {
const std::string DMC01("dmc01cpp");
const std::string DMC02("dmc02cpp");
} // namespace

class NeXusFileNapiTest : public CxxTest::TestSuite {

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static NeXusFileNapiTest *createSuite() { return new NeXusFileNapiTest(); }
  static void destroySuite(NeXusFileNapiTest *suite) { delete suite; }

private:
  void do_test_write(const string &filename, NXaccess create_code) {
    cout << "writeTest(" << filename << ") started\n";
    NeXus::File file(filename, create_code);
    // create group
    file.makeGroup("entry", "NXentry", true);
    // group attributes
    file.putAttr("hugo", "namenlos");
    file.putAttr("cucumber", "passion");
    // put string
    file.writeData("ch_data", "NeXus_data");

    // 2d array
    vector<int> array_dims;
    array_dims.push_back(5);
    array_dims.push_back(4);
    char c1_array[5][4] = {
        {'a', 'b', 'c', 'd'}, {'e', 'f', 'g', 'h'}, {'i', 'j', 'k', 'l'}, {'m', 'n', 'o', 'p'}, {'q', 'r', 's', 't'}};
    file.makeData("c1_data", NXnumtype::CHAR, array_dims, true);
    file.putData(&c1_array);
    file.closeData();

    // 1d uint8 array
    vector<uint8_t> i1_array;
    for (uint8_t i = 0; i < 4; i++) {
      i1_array.push_back(static_cast<uint8_t>(i + 1));
    }
    file.writeData("i1_data", i1_array);

    // 1d int16 array
    vector<int16_t> i2_array;
    for (int16_t i = 0; i < 4; i++) {
      i2_array.push_back(static_cast<int16_t>(1000 * (i + 1)));
    }
    file.writeData("i2_data", i2_array);

    // 1d int32 data
    vector<int32_t> i4_array;
    for (int32_t i = 0; i < 4; i++) {
      i4_array.push_back(1000000 * (i + 1));
    }
    file.writeData("i4_data", i4_array);

    // 2d float data
    vector<float> r4_array;
    for (size_t i = 0; i < 5 * 4; i++) {
      r4_array.push_back(static_cast<float>(i));
    }
    file.writeData("r4_data", r4_array, array_dims);

    // 2d double data - slab test
    vector<double> r8_array;
    for (size_t i = 0; i < 5 * 4; i++) {
      r8_array.push_back(static_cast<double>(i + 20));
    }
    file.makeData("r8_data", NXnumtype::FLOAT64, array_dims, true);
    vector<int> slab_start;
    slab_start.push_back(4);
    slab_start.push_back(0);
    vector<int> slab_size;
    slab_size.push_back(1);
    slab_size.push_back(4);
    file.putSlab(&(r8_array[16]), slab_start, slab_size);
    slab_start[0] = 0;
    slab_start[1] = 0;
    slab_size[0] = 4;
    slab_size[1] = 4;
    file.putSlab(&(r8_array[0]), slab_start, slab_size);

    // add some attributes
    cout << "writing attributes to r8_data" << std::endl;
    file.putAttr("ch_attribute", "NeXus");
    file.putAttr("i4_attribute", 42);
    file.putAttr("r4_attribute", 3.14159265);
    cout << "... done" << std::endl;

    // set up for creating a link
    NXlink link = file.getDataID();
    file.closeData();

    // int64 tests
#if HAVE_LONG_LONG_INT
    vector<int64_t> grossezahl{12, 555555555555LL, 23, 777777777777LL};
#else
    vector<int64_t> grossezahl{12, 555555, 23, 77777};
#endif
    if (create_code != NXACC_CREATE4) {
      file.writeData("grosszahl", grossezahl);
    }

    // create a new group inside this one
    file.makeGroup("data", "NXdata", true);

    // create a link
    file.makeLink(link);

    // compressed data
    array_dims[0] = 100;
    array_dims[1] = 20;
    vector<int> comp_array;
    for (int i = 0; i < array_dims[0]; i++) {
      for (int j = 0; j < array_dims[1]; j++) {
        comp_array.push_back(i);
      }
    }
    vector<int> cdims;
    cdims.push_back(20);
    cdims.push_back(20);
    file.writeCompData("comp_data", comp_array, array_dims, NeXus::LZW, cdims);

    // ---------- Test write Extendible Data --------------------------
    std::vector<int> data(10, 123);
    file.makeGroup("extendible_data", "NXdata", 1);
    file.writeExtendibleData("mydata1", data);
    file.writeExtendibleData("mydata2", data, 1000);
    std::vector<int64_t> dims(2);
    dims[0] = 5;
    dims[1] = 2;
    std::vector<int64_t> chunk(2, 2);
    file.writeExtendibleData("my2Ddata", data, dims, chunk);
    file.putAttr("string_attrib", "some short string");

    // Data vector can grow
    for (size_t i = 0; i < 6; i++)
      data.push_back(456);
    data[0] = 789;
    file.writeUpdatedData("mydata1", data);

    dims[0] = 8;
    dims[1] = 2;
    file.writeUpdatedData("my2Ddata", data, dims);

    // Data vector can also shrink!
    data.clear();
    data.resize(5, 234);
    file.writeUpdatedData("mydata2", data);

    // Exit the group
    file.closeGroup();
    // ---------- End Test write Extendible Data --------------------------

    // simple flush test
    file.flush();

    // real flush test
    file.makeData("flush_data", NeXus::getType<int>(), NX_UNLIMITED, true);
    vector<int> slab_array;
    slab_array.push_back(0);
    for (int i = 0; i < 7; i++) {
      slab_array[0] = i;
      file.putSlab(slab_array, i, 1);
      file.flush();
      file.openData("flush_data");
    }
    file.closeData();
    file.closeGroup();

    // create a sample
    file.makeGroup("sample", "NXsample", true);
    file.writeData("ch_data", "NeXus sample");

    // make more links
    NXlink glink = file.getGroupID();
    file.openPath("/");
    file.makeGroup("link", "NXentry", true);
    file.makeLink(glink);
    cout << "writeTest(" << filename << ") successful\n";

    TS_ASSERT_EQUALS(std::filesystem::exists(filename), true);
  }

  void do_test_read(const string &filename) {
    cout << "readTest(" << filename << ") started\n";
    const string SDS("SDS");
    // top level file information
    NeXus::File file(filename);
    file.openGroup("entry", "NXentry");

    // Test getDataCoerce() -------------------
    std::vector<int> ints;
    std::vector<double> doubles;

    ints.clear();
    file.openData("i1_data");
    file.getDataCoerce(ints);
    TS_ASSERT_EQUALS(ints.size(), 4);
    TS_ASSERT_EQUALS(ints[0], 1);
    file.closeData();

    ints.clear();
    file.openData("i2_data");
    file.getDataCoerce(ints);
    TS_ASSERT_EQUALS(ints.size(), 4);
    TS_ASSERT_EQUALS(ints[0], 1000);
    file.closeData();

    ints.clear();
    file.openData("i4_data");
    file.getDataCoerce(ints);
    TS_ASSERT_EQUALS(ints.size(), 4);
    TS_ASSERT_EQUALS(ints[0], 1000000)
    file.closeData();

    doubles.clear();
    file.openData("r4_data");
    file.getDataCoerce(doubles);
    TS_ASSERT_EQUALS(doubles.size(), 20);
    TS_ASSERT_EQUALS(doubles[1], 1.0)
    file.closeData();

    doubles.clear();
    file.openData("r8_data");
    file.getDataCoerce(doubles);
    TS_ASSERT_EQUALS(doubles.size(), 20);
    TS_ASSERT_EQUALS(doubles[1], 21.0)
    file.closeData();

    // Throws when you coerce to int from a real/double source
    ints.clear();
    file.openData("r8_data");
    TS_ASSERT_THROWS_ANYTHING(file.getDataCoerce(ints));
    file.closeData();

    // Close the "entry" group
    file.closeGroup();

    // openpath checks
    file.openPath("/entry/data/comp_data");
    file.openPath("/entry/data/comp_data");
    file.openPath("../r8_data");
    cout << "NXopenpath checks OK\n";

    // everything went fine
    cout << "readTest(" << filename << ") successful\n";
  }

  void do_test_loadPath(const string &filename) {
    if (getenv("NX_LOAD_PATH") != NULL) {
      TS_ASSERT_THROWS_NOTHING(NeXus::File file(filename, NXACC_RDWR));
      cout << "Success loading NeXus file from path" << endl;
    } else {
      cout << "NX_LOAD_PATH variable not defined. Skipping testLoadPath\n";
    }
  }

public:
  void test_readwrite_hdf5() {
    cout << " Nexus File Tests\n";
    NXaccess const nx_creation_code = NXACC_CREATE5;
    string const fileext = ".h5";
    string const filename("nexus_file_napi_test_cpp" + fileext);

    removeFile(filename); // in case last round failed

    // try writing a file
    do_test_write(filename, nx_creation_code);

    // try reading a file
    do_test_read(filename);

    removeFile(filename); // cleanup

    // try using the load path
    do_test_loadPath(DMC01 + fileext);
    do_test_loadPath(DMC02 + fileext);

    removeFile(DMC01 + fileext);
    removeFile(DMC02 + fileext);
  }
};
